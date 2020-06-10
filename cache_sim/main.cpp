#include <fstream>
#include <iostream>

struct	inst_t
{
	char type;
	uint32_t value;
}

inst_t inst_mem[64];

int main(int argc, char** argv)
{

	std::string path = "./trace1.txt";


	std::ifstream file(path.data());
	if(file.is_open())
	{
		std::string line;
		while(getline(file, line))
		{
			std::cout << line << std::endl;
		}
		file.close();
	}
}
