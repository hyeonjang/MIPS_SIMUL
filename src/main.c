#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#pragma warning(disable:4996)

#define MEMSIZE 4096

typedef uint32_t	uint;

typedef struct fileInfo_t
{
	uint fileSize;
	uint n_inst;
} fileInfo;

typedef union MIPS_t  //It is litte Endian
{
	char input[4];
	
	struct RFormat_t
	{
		uint funct : 6; //5bits
		uint shamt : 5; //5bits
		uint rd : 5; //6bits	
		uint rt : 5; //5bits 
		uint rs : 5; //5bits
		uint op : 6; //6bits
	}R;

	struct IFormat_t
	{
		int16_t immediate : 16; //16bits, 2bytes
		uint rt : 5; //5bits, 1bytes
		uint rs : 5; //5bits, 1bytes
		uint op : 6;
	}I;

	struct jump_t
	{
		int address : 26;
		uint op : 6;
	}J;

} MIPS;

int32_t reg[32];
struct reg64_t
{ 
	int32_t HI;
	int32_t LO;
}reg64;
uint32_t* inst_memory;
uint32_t* data_memory;
uint32_t PC, i_mem_start_point, d_mem_start_point;

fileInfo setFileInfo(FILE* file)
{
	fileInfo info;	
	
	// estimate whole file size
	fseek(file, 0, SEEK_END);
	info.fileSize = ftell(file);
	rewind(file);

	info.n_inst = info.fileSize/sizeof(int32_t);

	return info;
}

void printInst(MIPS mips, int instIdx)
{
	printf("inst %d: %08x ", instIdx, mips);

	switch (mips.I.op)
	{
		//not i format
	case 0b000000:/*printf("sll"); break;*/ goto RFORMAT; 

		//Arithmetic and Logical Instructions
	case 0b001000: printf("addi");	break;
	case 0b001001: printf("addiu");	break;
	case 0b001100: printf("andi");	break;
	case 0b001101: printf("ori");	break;
	case 0b001110: printf("xori");	break;
		//comparion instructions
	case 0b001010: printf("slti");	break;
	case 0b001011: printf("sltiu");	break;
		//branch instructions
	case 0b000100: printf("beq");	printf(" $%d, $%d, %d\n", mips.I.rs, mips.I.rt, mips.I.immediate); return;
	case 0b000101: printf("bne");	printf(" $%d, $%d, %d\n", mips.I.rs, mips.I.rt, mips.I.immediate); return;
		//load instructions
	case 0b100000: printf("lb");	goto LOAD_IFORMAT;
	case 0b100100: printf("lbu");	goto LOAD_IFORMAT;
	case 0b100001: printf("lh");	goto LOAD_IFORMAT;
	case 0b100101: printf("lhu");	goto LOAD_IFORMAT;
	case 0b100011: printf("lw");	goto LOAD_IFORMAT;
		//should be checked
	case 0b001111: printf("lui");	
		printf(" $%d, %x\n", mips.I.rt, ((uint32_t)mips.I.immediate<<16)); return; 
		//store instructions			
	case 0b101000: printf("sb");	goto LOAD_IFORMAT;
	case 0b101001: printf("sh");	goto LOAD_IFORMAT;
	case 0b101011: printf("sw");	goto LOAD_IFORMAT;
		//jump instructions
	case 0b000010: printf("j");	printf(" %d\n", mips.J.address); return;
	case 0b000011: printf("jal");	printf(" %d\n", mips.J.address); return;

	default: goto UNKWON;
	}
IFORMAT:
	printf(" $%d, $%d, %0x\n", mips.I.rt, mips.I.rs, mips.I.immediate);
	return;
LOAD_IFORMAT:
	printf(" $%d, %d($%d)\n", mips.I.rt, mips.I.immediate, mips.I.rs);
	return;
RFORMAT:
	switch (mips.R.funct)
	{
	case 0b000000: printf("sll");	goto SHIFT_RFORMAT;
	case 0b000010: printf("srl");	goto SHIFT_RFORMAT;
	case 0b000011: printf("sra");	goto SHIFT_RFORMAT;
		//Arithmetic and Logical Instructions
	case 0b100000: printf("add");	break;
	case 0b100001: printf("addu");	break;
	case 0b100010: printf("sub");	break;
	case 0b100011: printf("subu");	break;
	case 0b011010: printf("div");	printf(" $%d, $%d\n", mips.R.rs, mips.R.rt); return;
	case 0b011011: printf("divu");	printf(" $%d, $%d\n", mips.R.rs, mips.R.rt); return;
	case 0b011000: printf("mult");	printf(" $%d, $%d\n", mips.R.rs, mips.R.rt); return;
	case 0b011001: printf("multu");	printf(" $%d, $%d\n", mips.R.rs, mips.R.rt); return;
	case 0b100100: printf("and");	break;
	case 0b100101: printf("or");	break;
	case 0b100110: printf("xor");	break;
	case 0b100111: printf("nor");	break;
	case 0b000100: printf("sllv");	goto SHIFTV_FORMAT;
	case 0b000110: printf("srlv");	goto SHIFTV_FORMAT;
	case 0b000111: printf("srav");	goto SHIFTV_FORMAT;
		//comparion instructions
	case 0b101010: printf("slt");	break;
	case 0b101011: printf("sltu");	break;
		//load instructions
	case 0b010000: printf("mfhi");	printf(" $%d\n", mips.R.rd); return;
	case 0b010010: printf("mflo");	printf(" $%d\n", mips.R.rd); return;
	case 0b010001: printf("mthi");	printf(" $%d\n", mips.R.rs); return;
	case 0b010011: printf("mtlo");	printf(" $%d\n", mips.R.rs); return;
		//jump instructions
	case 0b001000: printf("jr");	printf(" $%d\n", mips.R.rs); return;
	case 0b001001: printf("jalr");	printf(" $%d\n", mips.R.rs); return;
		//syscall
	case 0b001100: printf("syscall\n"); return;

	default: goto UNKWON;
	}
	printf(" $%d, $%d, $%d\n", mips.R.rd, mips.R.rs, mips.R.rt);
	return;
SHIFT_RFORMAT:
	printf(" $%d, $%d, %d\n", mips.R.rd, mips.R.rt, mips.R.shamt); 
	return;
SHIFTV_FORMAT:
	printf(" $%d, $%d, %d\n", mips.R.rd, mips.R.rt, mips.R.rs); 
	return;
UNKWON:
	printf("unknown instruction\n");
	return;
}
//*************************************************
//proj2
void initReg()
{
	for(int i=0;i<sizeof(reg);i++) reg[i] = 0x00000000;
}

uint32_t* initMem()
{
	inst_memory = malloc(sizeof(uint32_t)*MEMSIZE);
	for(int i=0;i<MEMSIZE;i++) inst_memory[i]=0xffffffff;
	PC=i_mem_start_point = inst_memory-inst_memory;

	data_memory = malloc(sizeof(uint32_t)*MEMSIZE);
	for(int i=0;i<MEMSIZE;i++) data_memory[i]=0xffffffff;
	d_mem_start_point = data_memory-data_memory+0x10000000;
}

void loadInst( MIPS mips, int idx ){ memcpy( &inst_memory[idx], &mips, sizeof(MIPS) ); }
void loadData( char* data, fileInfo info )
{ 
	for(int i=0; i<info.n_inst+1; i++)
		memcpy(&data_memory[i], &data[4*i], sizeof(int32_t));
}

MIPS readInstfromFile(FILE* file)
{
	MIPS mips;

	char* input = (char*)malloc(sizeof(char)*4);
	int result = fread(input, sizeof(char), 4, file);
	memcpy(&mips.input[0], &input[3], sizeof(MIPS));
	memcpy(&mips.input[1], &input[2], sizeof(MIPS));
	memcpy(&mips.input[2], &input[1], sizeof(MIPS));
	memcpy(&mips.input[3], &input[0], sizeof(MIPS));
	free(input);

	return mips;
}

char* readDatafromFile(FILE* file, fileInfo info)
{
	int n_inst = info.n_inst;

	char* input = (char*)malloc(sizeof(char)*4);
	char* data  = (char*)malloc(sizeof(int32_t)*(n_inst+1));

	for(int i=0; i<n_inst; i++)
	{
		int result = fread(input, sizeof(char), 4, file);
		memcpy(&data[4*i],   &input[3], sizeof(char));
		memcpy(&data[4*i+1], &input[2], sizeof(char));
		memcpy(&data[4*i+2], &input[1], sizeof(char));
		memcpy(&data[4*i+3], &input[0], sizeof(char));
	}

	int remain = info.fileSize%sizeof(int32_t);
	if(!remain){ free(input);return data; }

	data[4*n_inst]=0xff; data[4*n_inst+1]=0xff; data[4*n_inst+2]=0xff; data[4*n_inst]=0xff;

	for(int i=0; i<remain; i++)
	{	
		int result = fread(input, sizeof(char), 1, file);
		memcpy(&data[4*n_inst+3-i], &input[0], sizeof(char));
	}
	free(input);

	return data;
}


bool run(int idx)
{
	MIPS mips;

	PC += 4;
	if(inst_memory[idx]==0xffffffff) goto UNKWON;
	memcpy(&mips, &inst_memory[idx], sizeof(MIPS));

	//***************************************
	// the variable for load data instruction 
	int32_t reg_mips_I_rs, d_mem_idx;
	if((mips.I.op>>3)==0b100) 
	{
		reg_mips_I_rs = d_mem_start_point - reg[mips.I.rs];
		d_mem_idx = mips.I.immediate>>2+reg_mips_I_rs;
	}
	//***************************************

	switch (mips.I.op)
	{
		//not i format
	case 0b000000:/*printf("sll"); break;*/ goto RFORMAT; 

		//Arithmetic and Logical Instructions
	case 0b001001:/*addi*/ reg[mips.I.rt]=reg[mips.I.rs]+mips.I.immediate; break;
	case 0b001000:/*addiu*/reg[mips.I.rt]=reg[mips.I.rs]+(uint32_t)mips.I.immediate; break;
	case 0b001100:/*andi*/ reg[mips.I.rt]=reg[mips.I.rs]&mips.I.immediate; break;
	case 0b001101:/*ori"*/ reg[mips.I.rt]=reg[mips.I.rs]|(uint32_t)mips.I.immediate; break;
	case 0b001110:/*xori*/ reg[mips.I.rt]=reg[mips.I.rs]^mips.I.immediate; break;
		//comparion instructions
	case 0b001010:/*slti*/ reg[mips.I.rt]=(reg[mips.I.rs]<mips.I.immediate)?0x1:0x0; break;
	case 0b001011:/*sltui*/reg[mips.I.rt]=(reg[mips.I.rs]<(uint32_t)mips.I.immediate)?0x1:0x0; break;
		//branch instructions
	case 0b000100:/*beq*/ return true;
	case 0b000101:/*bne*/ return true;
		//load instructions
	case 0b100000:/*lb*/ goto LOAD_IFORMAT;
	case 0b100100:/*lbu*/goto LOAD_IFORMAT;
	case 0b100001:/*lh*/ goto LOAD_IFORMAT;
	case 0b100101:/*lhu*/goto LOAD_IFORMAT;
	case 0b100011:/*lw*/ reg[mips.I.rt]=data_memory[d_mem_idx]; goto LOAD_IFORMAT;
		//should be checked
	case 0b001111:/*lui*/ 
			reg[mips.I.rt] = (((uint32_t)mips.I.immediate)<<16); 
			printf(" $%d, %x\n", mips.I.rt, ((uint32_t)mips.I.immediate<<16));
			return true;
		//store instructions			
	case 0b101000:/*sb*/ goto LOAD_IFORMAT;
	case 0b101001:/*sh*/ goto LOAD_IFORMAT;
	case 0b101011:/*sw*/ goto LOAD_IFORMAT;
		//jump instructions
	case 0b000010:/*j*/   return true;
	case 0b000011:/*jal*/ return true;

	default: goto UNKWON;
	}
IFORMAT:
	 
	return true;
LOAD_IFORMAT:
	
	return true;
RFORMAT:
	//uint64_t HILO;
	switch (mips.R.funct)
	{
	case 0b000000:/*sll*/ reg[mips.R.rd]=(uint32_t)reg[mips.R.rt]<<mips.R.shamt; goto SHIFT_RFORMAT;
	case 0b000010:/*srl*/ reg[mips.R.rd]=(uint32_t)reg[mips.R.rt]>>mips.R.shamt; goto SHIFT_RFORMAT;
	case 0b000011:/*sra*/ reg[mips.R.rd]=reg[mips.R.rt]>>mips.R.shamt; goto SHIFT_RFORMAT;
		//Arithmetic and Logical Instructions
	case 0b100000:/*add*/  reg[mips.R.rd]=reg[mips.R.rs]+reg[mips.R.rt]; break;
	case 0b100001:/*addu*/ reg[mips.R.rd]=reg[mips.R.rs]+reg[mips.R.rt]; break;
	case 0b100010:/*sub*/  reg[mips.R.rd]=reg[mips.R.rs]-reg[mips.R.rt]; break;
	case 0b100011:/*subu*/ reg[mips.R.rd]=reg[mips.R.rs]-reg[mips.R.rt]; break;
	case 0b011010:/*div*/  reg64.HI=reg[mips.R.rs]%reg[mips.R.rt]; reg64.LO=reg[mips.R.rs]/reg[mips.R.rt]; break;
	case 0b011011:/*divu*/ 
		reg64.HI=(uint32_t)reg[mips.R.rs]%reg[mips.R.rt]; 
		reg64.LO=(uint32_t)reg[mips.R.rs]/reg[mips.R.rt]; break;
	case 0b011000:/*mult*/  
		reg64.HI=((reg[mips.R.rs]*(reg[mips.R.rt]>>16))&-1)>>16;
		reg64.LO=((reg[mips.R.rs]*reg[mips.R.rt])&-1); 
		return true;
	case 0b011001:/*multu*/
		reg64.HI=((reg[mips.R.rs]*((uint32_t)reg[mips.R.rt]>>16))&-1)>>16;
		reg64.LO=((reg[mips.R.rs]*reg[mips.R.rt])&-1); 
		return true;
	case 0b100100:/*and*/  reg[mips.R.rd]=reg[mips.R.rs]&reg[mips.R.rt]; break;
	case 0b100101:/*or"*/  reg[mips.R.rd]=reg[mips.R.rs]|reg[mips.R.rt]; break;
	case 0b100110:/*xor*/  reg[mips.R.rd]=reg[mips.R.rs]^reg[mips.R.rt]; break;
	case 0b100111:/*nor*/  reg[mips.R.rd]=~(reg[mips.R.rs]|reg[mips.R.rt]); break;
	case 0b000100:/*sllv*/ reg[mips.R.rd]=(uint32_t)reg[mips.R.rt]<<reg[mips.R.rs]; break;
	case 0b000110:/*srlv*/ reg[mips.R.rd]=(uint32_t)reg[mips.R.rt]>>reg[mips.R.rs]; break;
	case 0b000111:/*srav*/ reg[mips.R.rd]=reg[mips.R.rt]>>reg[mips.R.rs]; break;
		//comparion instructions
	case 0b101010:/*slt*/  reg[mips.R.rd]=(reg[mips.R.rs]<reg[mips.R.rt])?0x1:0x0; break;
	case 0b101011:/*sltu*/ reg[mips.R.rd]=(reg[mips.R.rs]<(uint32_t)reg[mips.R.rt])?0x1:0x0; break;
		//load instructions
	case 0b010000:/*mfhi*/ reg[mips.R.rd]=reg64.HI; return true;
	case 0b010010:/*mflo*/ reg[mips.R.rd]=reg64.LO; return true;
	case 0b010001:/*mthi*/ reg64.HI=reg[mips.R.rs]; return true;
	case 0b010011:/*mtlo*/ reg64.LO=reg[mips.R.rs]; return true;
		//jump instructions
	case 0b001000:/*jr*/   return true;
	case 0b001001:/*jalr*/ return true;
		//syscall
	case 0b001100:/*syscall*/ return true;

	default: goto UNKWON;
	}
	return true;
SHIFT_RFORMAT: return true;
UNKWON:
	printf("unknown instruction\n");
	return false;
}


int main(int argc, char *argv[])
{
	//initialize regsiters and instructions Memory
	initReg();
	initMem();	
	
	char input[100];
INPUT:
	printf("mips-sim> "); gets(input);
	int i = 0;
	while(input[i] != NULL && input[i] != ' ') i++;
	
	//*********************************************
	//
	if(!strncmp(&input[0], "exit", 4)) return 0;
	else if(!strncmp(&input[0], "read", 4))
	{	
		FILE* pFile = fopen(&input[++i], "rb");
		if (pFile == NULL){ printf("%s not found. please check filepath\n", &input[i]); goto INPUT;}
		else
		{
			fileInfo info = setFileInfo(pFile);	

			for(int i = 0; i < info.n_inst; i++)
			{
				MIPS mipsBuf = readInstfromFile(pFile);
				printInst(mipsBuf, i);
			}

			fclose(pFile);
			goto INPUT;
		}
	}
	//**********************************************
	// end of proj1
	//**********************************************
	// start of proj2,3
	else if(!strncmp(&input[0], "loadinst", 8))
	{	
		FILE* pFile = fopen(&input[++i], "rb");
		if (pFile == NULL){ printf("%s not found. please check filepath\n", &input[i]); goto INPUT;}
		else
		{
			fileInfo info = setFileInfo(pFile);	
			for(int i = 0; i < info.n_inst; i++)
			{
				MIPS mipsBuf = readInstfromFile(pFile);
				loadInst(mipsBuf, i);
			}
			fclose(pFile);
			goto INPUT;
		}
	}	
	else if(!strncmp( &input[0], "loaddata", 8 ))
	{
		FILE* pFile = fopen( &input[++i], "rb" );
		if(pFile==NULL){ printf( "%s not found. please check filepath\n", &input[i] ); goto INPUT; }
		else
		{
			fileInfo info = setFileInfo( pFile );
			
			char* data = readDatafromFile( pFile, info );
			loadData( data, info );
			free(data);

			fclose( pFile );
			goto INPUT;
		}
	}
	else if(!strncmp(&input[0], "run", 3))
	{
		int inputN = atoi(&input[i++]);
		int excuted = 0;
		for(int k=0;k<inputN;k++)
		{ 	
			excuted++;
			if(!run(k)) break;
		}
		printf("Executed %d instructions\n", excuted);
		free(inst_memory);
		free(data_memory);
		goto INPUT;
	}
	else if(!strncmp(&input[0], "registers", 9))
	{
		for(int i = 0; i < 32; i ++) printf("$%d: 0x%08x\n", i, reg[i]);
		printf( "HI: 0x%08x\n", reg64.HI );
		printf( "LO: 0x%08x\n", reg64.LO );
		printf( "PC: 0x%08x\n", PC );
		goto INPUT;
	}
//***********************************************
//debugging code for proj3
	else if(!strncmp(&input[0], "memory", 6))
	{
		for(int i=0;i<MEMSIZE;i++) printf("0x%08x: 0x%08x\n", &inst_memory[i], inst_memory[i]);
		goto INPUT;
	}
	else if(!strncmp(&input[0], "proj3", 5))
	{
		char* t_inst_str = "../_test/proj3/test1_t.dat";
		char* t_data_str = "../_test/proj3/test2_d.dat";

		FILE* pFile = fopen( t_inst_str, "rb" );
		if (pFile == NULL){ printf("%s not found. please check filepath\n", &input[i]); goto INPUT;}
		else
		{
			fileInfo info = setFileInfo(pFile);	

			for(int i = 0; i < info.n_inst; i++)
			{
				MIPS mipsBuf = readInstfromFile(pFile);
				printInst(mipsBuf, i);
			}

			fclose(pFile);
	 	}
		pFile = fopen( t_inst_str, "rb");
		if (pFile == NULL){ printf("%s not found. please check filepath\n", &input[i]); goto INPUT;}
		else
		{
			fileInfo info = setFileInfo(pFile);	
			for(int i = 0; i < info.n_inst; i++)
			{
				MIPS mipsBuf = readInstfromFile(pFile);
				loadInst(mipsBuf, i);
			}
			fclose(pFile);
		}

		FILE* d_File = fopen( t_data_str, "rb" );
		if(pFile==NULL){ printf( "%s not found. please check filepath\n", &input[i] ); goto INPUT; }
		else
		{
			fileInfo info = setFileInfo( d_File );
			
			char* data = readDatafromFile( d_File, info );
			loadData( data, info );
			free(data);

			fclose( d_File );
		}
		
		//for(int i=0;i<MEMSIZE;i++) printf("0x%08x: 0x%08x\n", &inst_memory[i], inst_memory[i]);
		for(int i=0;i<4;i++) printf("0x%08x: 0x%08x\n", &data_memory[i], data_memory[i]);

		int inputN = 50; //atoi(&input[i++]);
		int excuted = 0;
		for(int k=0;k<inputN;k++)
		{ 	
			excuted++;
			if(!run(k)) break;
		}
		printf("Executed %d instructions\n", excuted);
		free(inst_memory);
		free(data_memory);
		for(int i = 0; i < 32; i ++) printf("$%d: 0x%08x\n", i, reg[i]);
		printf( "HI: 0x%08x\n", reg64.HI );
		printf( "LO: 0x%08x\n", reg64.LO );
		printf( "PC: 0x%08x\n", PC );
	}

//*********************************************
	else 
	{ 
		printf("usage: 'exit' or 'read filename'\n"); 
		goto INPUT;
	}
}
