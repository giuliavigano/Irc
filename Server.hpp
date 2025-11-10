#ifndef	SERVER_HPP
# define SERVER_HPP
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>

#include "Client.hpp"
#include "Channel.hpp"


class Server {
public:
	Server(int _port, const std::string& password);
	~Server();

	bool	init();
	void	run();
private:
	int						port;
	std::string				passWord;
	int						socketFd;
	std::map<int , Client*>	clients;
};

#endif