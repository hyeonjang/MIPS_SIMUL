#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

using uint=unsigned int;

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
	struct t0{ uint offset:4; uint index:10; uint tag:18; };
	struct t1{ uint offset:6; uint index:10; uint tag:16; };
	union{ int32_t addr; t0 _0; t1 _1; };

	// constructors
	inst_t(){};
	inst_t(char t, int32_t buf) { type=t; addr=buf; };
};

//struct mem_t
//{
//	std::vector<int> data;
//	__forceinline int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
//} mem;

struct block_t
{
	bool valid;
	bool dirty;
	int32_t tag;
	int data[4];

	block_t(){ valid=0; dirty=0; tag=0; }

	template<class T> bool compare(T inst){ if(!valid) return false; if(tag!=inst.tag) return false; return true; }
	template<class T> void fetch(T inst){ valid=1; tag=inst.tag; dirty=0; /*data[0]=mem[0];*/ }

	// write back features
	template<class T> bool is_dirty(T inst){ return dirty; }
	template<class T> void set_dirty(T inst, bool _t){ dirty=_t; }
};

struct cache_t0
{
	block_t b[1024];

	//functions
	template<class T> bool compare(T inst) { return b[inst.index].compare(inst); }
	template<class T> void fetch(T inst){ b[inst.index].fetch(inst); }
	
	// overrided operators
	__forceinline int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
};

struct set_t2 
{	
	bool LRU; 
	block_t b[2];	
	set_t2():LRU(0){}

	template<class T> bool compare(T inst) { return b[0].compare(inst)||b[1].compare(inst); }
	template<class T> void fetch(T inst) { b[LRU].fetch(inst); LRU=!LRU; }
	template<class T> bool is_dirty(T inst){ return b[LRU].is_dirty(inst); }
	template<class T> void set_dirty(T inst, bool _t){ b[LRU].set_dirty(inst, _t); LRU=!LRU; }
};
struct cache_t1
{
	set_t2 set[4096];

	template<class T> bool compare(T inst){ return set[inst.index].compare(inst); }
	template<class T> void fetch(T inst){ set[inst.index].fetch(inst); }
	template<class T> bool is_dirty(T inst){ return set[inst.index].is_dirty(inst); }
	template<class T> void set_dirty(T inst, bool _t){ set[inst.index].set_dirty(inst, _t); }
	
	// overrided operators
	__forceinline int& operator[]( ptrdiff_t i ){ return ((int*)this)[i]; }
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
	~cache_sim(){}

	// functions
	void load_inst(const char* path)
	{	
		char buff[8192];
		FILE* fp = fopen(path, "r");
		
		fileinfo info = setFileInfo(fp);
		insts.reserve(info.n_inst);
		for(uint i=0; i<info.n_inst; i++)
		{
			fgets(buff, 12, fp);
			inst_t inst = inst_t(buff[0], std::stoll(&buff[2], nullptr, 16));
			insts.emplace_back(inst);
		}
		fclose(fp);
	}

	void excute(int type){ printf("%d\n", uint(insts.size())); type==0?excute0():excute1(); insts.clear(); insts.shrink_to_fit(); }

	void excute0()
	{
		for(auto& inst : insts)
		{
			inst_t::t0 i = inst._0;
			if(inst.type=='L')
			{
				if(cache0.compare(i)) continue;
				cache0.fetch(i); miss++;
			}
			else if(inst.type=='S')
			{
				if(!cache0.compare(i)) miss++;
				write++;
			}
		}
	}

	void excute1()
	{
		for(auto& inst:insts)
		{
			inst_t::t1 i = inst._1;
			if(inst.type=='L')
			{
				if(cache1.compare(i)) continue; 
				cache1.fetch(i); miss++;
			}
			else if(inst.type=='S')
			{
				// write hit
				if(cache1.compare(i))
				{
					if(!cache1.is_dirty(i)) cache1.set_dirty(i, true);
					else if(cache1.is_dirty(i)) { write++; cache1.set_dirty(i, false); }
				}
				// write miss
				else if(!cache1.compare(i)) 
				{
					if(cache1.is_dirty(i)) { write++; cache1.set_dirty(i, false); }
					cache1.fetch(i); miss++;
				}
			}
		}
	}

	uint32_t return_miss() { return this->miss; }
	uint32_t return_write(){ return this->write; }
};

int main(int argc, char** argv)
{
	cache_sim cache;

	std::string trace = "./trace1.txt";	std::string sr(argv[0]);

	//printf("%s\n", trace.c_str());
	trace = trace+sr+".txt";
		
	cache.load_inst(trace.c_str());
	cache.excute(1);
	printf("%d %d\n", cache.return_miss(), cache.return_write());

}
