#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

struct	inst_t
{
	char type;
	int32_t value;
	
	inst_t(){ type = NULL, value = NULL; };

};

int main(int argc, char** argv)
{
	inst_t inst_mem[64];
	
	std::string path = "./trace1.txt";


	std::ifstream file(path.data());
	if(file.is_open())
	{
		std::string line;
		for(int i =0; i<sizeof(inst_mem), std::getline(file, line); i++)
		{
			inst_mem[i].type = line[0];
			std::cout << "origin " << line.substr(0, 10).c_str() << std::endl;
			std::cout << "ato "<< std::atoll(line.substr(1, 10).c_str()) << std::endl;
			inst_mem[i].value = std::atol(line.substr(1, -1).c_str());

			memcpy(&inst_mem[i], &line[2], sizeof(int32_t));

			printf("inst_mem %08x\n", inst_mem[i].value);
			
		}
		file.close();
	}
}
