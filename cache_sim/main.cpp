#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <byteswap.h>

struct fileinfo
{
	uint32_t fileSize;
	uint32_t n_inst;
};

fileinfo setFileInfo(FILE* file)
{
	fileinfo info;	
	
	// estimate whole file size
	fseek(file, 0, SEEK_END);
	info.fileSize = ftell(file);
	rewind(file);

	info.n_inst = info.fileSize/12;

	return info;
}

struct inst_t
{
	char type;
	struct t0{ int32_t offset:4; int32_t index:10; int32_t tag:18; };
	struct t1{ int32_t offset:6; int32_t index:10; int32_t tag:16; };
	union{ int32_t addr; t0 _0; t1 _1; };

	// constructors
	inst_t(){};
	inst_t(char t, uint32_t buf) { type=t; addr=buf; };
};

struct mem_t
{
	std::vector<int> data;
	// int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
} mem;

struct block_t
{
	bool valid;
	bool dirty;
	int32_t tag;
	int data[4];

	block_t(){ valid=0; dirty=0; tag=0; }

	template<class T> bool compare(T inst){ if(!valid) return false; if(tag!=inst.tag) return false; return true; };
	template<class T> void fetch(T inst){ valid=1; tag=inst.tag; /*data[0]=mem[0];*/ };
	
	// write back features
	template<class T> bool write(T inst){ if(dirty==0){ dirty=!dirty; return false; } else dirty=!dirty; return true; }
};

struct cache_t0
{
	block_t b[1024];

	//functions
	template<class T> bool compare(T inst) { return b[inst.index].compare(inst); }
	template<class T> void fetch(T inst){ b[inst.index].fetch(inst); }
	
	// overrided operators
	// int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
};

struct set_t2 
{	
	bool LRU; 
	block_t b[2];	
	set_t2():LRU(-1){}

	template<class T> bool compare(T inst) { return LRU=b[0].compare(inst)?0:b[1].compare(inst)?1:NULL; }
	template<class T> void fetch(T inst) { if(LRU==-1) return; b[!LRU].fetch(inst); LRU=!LRU; }
	template<class T> bool write(T inst){ return b[!LRU].write(inst); }
};
struct cache_t1
{
	set_t2 set[4096];

	template<class T> bool compare(T inst){ return set[inst.index].compare(inst); }
	template<class T> void fetch(T inst){ set[inst.index].fetch(inst); }
	template<class T> bool write(T inst){ return set[inst.index].write(inst); }
	
	// overrided operators
	// int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
};

class cache_sim
{
	std::vector<inst_t>	insts;
	cache_t0	cache0;
	cache_t1	cache1;

	uint32_t miss = 0;
	uint32_t write = 0;

public:
	// constructor
	cache_sim(){}

	// functions
	void load_inst(const char* path)
	{	
		char buff[8192];
		FILE* fp = fopen(path, "r");
		
		fileinfo info = setFileInfo(fp);
		for(int i=0; i<info.n_inst; i++)
		{
			fgets(buff, 12, fp);
			inst_t inst = inst_t(buff[0], uint32_t(std::stoi(&buff[2], nullptr, 16)));
			insts.emplace_back(inst);
		}
		fclose(fp);
	}

	void excute0()
	{
		for(auto& inst : insts)
		{
			printf("inst %c %0x\n", inst.type, inst.addr);
			inst_t::t0 i = inst._0;
			if(inst.type=='L')
			{
				if(cache0.compare(i)) continue;
				cache0.fetch(i); miss++;
			}
			else if(inst.type=='S')	write++;
		}
	}

	void excute1()
	{
		printf("%d\n", int(insts.size()));
		for(auto& inst:insts)
		{
			inst_t::t1 i = inst._1;
			if(inst.type=='L')
			{
				if(cache1.compare(i)) continue; 
				cache1.fetch(i);  miss++;
			}
			else if(inst.type=='S')
			{
				if(cache1.compare(i)) continue;
				if(cache1.write(i)) write++;
				miss++; 
			}
		}
	}

	uint32_t return_miss() { return this->miss; }
	uint32_t return_write(){ return this->write; }
};

int main(int argc, char** argv)
{
	cache_sim cache;
	cache.load_inst("./trace1.txt");
	cache.excute0();
	printf("%d %d\n", cache.return_miss(), cache.return_write());

}
