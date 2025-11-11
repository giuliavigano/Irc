#include "Server.hpp"
#include "Client.hpp"


int		main(int argc, char *argv[]) {
	if (argc != 3)
		std::cout << "Bad input, must be in this form: ./.. <port> <password> !" << std::endl;
//	parsing input ?
	Server	server(std::atoi(argv[1]), argv[2]);
	server.run();
}