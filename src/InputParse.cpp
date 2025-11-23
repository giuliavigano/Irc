#include "InputParse.hpp"
#include <iostream>
#include <cstdlib>

static void	portCheck(int port)
{
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	if (port < 1024)
		std::cerr << "Warning: Port " << port << " is a privileged port (requires root)." << std::endl;
}

void	InputParse::checkInput(int port, const std::string& psw)
{
	portCheck(port);
	
	if (psw.empty())
	{
		std::cerr << "Error: Password cannot be empty." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < psw.length(); i++)
	{
		// 33-126 range ascii visible chars excluding whitespace
		unsigned int	c = psw[i];
		if (c < 33 || c > 126)
		{
			std::cerr << "Error: invalid password" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}