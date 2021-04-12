#include "BlurringFilter.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>


int main(int argc, char** argv)
{
	try
	{
		// Parsing options
		std::vector<std::string> args(argv + 1, argv + argc);
		std::string in_file_path, out_file_path;
		float factor;

		if (args.size() != 3)
		{
			char buffer[50];
			sprintf_s(buffer, "Error: Found %d arguments, a number of 3 is expected", args.size());
			throw std::invalid_argument(buffer);
		}

		for (std::size_t i = 0; i < args.size(); i++)
		{
			if (args[i] == "-h" || args[i] == "--help")
			{
				std::cout << "Syntax: BlurringFilter -f <blur_factor> -i <infile> -o <outfile>" << std::endl;
				return 0;
			}
			else if (args[i] == "-f")
			{
				factor = std::stof(args[i + 1]);
			}
			else if (args[i] == "-i")
			{
				in_file_path = args[i + 1];
			}
			else if (args[i] == "-o")
			{
				out_file_path = args[i + 1];
			}
		}

		TGA* img = new TGA(in_file_path);
		img->blur(factor);
		img->write(out_file_path);

		return 0;
	}
	catch (std::exception& e)
	{
		std::cerr << "Caught: " << e.what() << std::endl;
		std::cerr << "Type: " << typeid(e).name() << std::endl;

		return 1;
	};
}
