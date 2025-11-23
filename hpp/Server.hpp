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
#include <sys/select.h>

#include "Client.hpp"
#include "HandleBuffer.hpp"
#include "Channel.hpp"


class Server {
public:
	Server(int _port, const std::string& password);
	~Server();

	bool	init();
	void	run();
	int		get_maxFd() const;
	
	std::string							getPsw() const;
	std::map<int, Client*>				getClientsMap() const;
	std::map<std::string, Channel*>&	getChannelsMap();
	const std::string&					getServerName() const;

	void	handleClient(int client_fd);
	void	handleNewConnection();
	void	sendReply(const std::string& code, const std::string& message, Client& client);
	void	handleClientWrite(Client* client);
	void	eraseClient(Client* client);

	void	ifRemoveChannel(const std::string& channelName);

private:
	int								port;
	std::string						passWord;
	int								socketFd;
	std::map<int , Client*>			clients;
	std::map<std::string, Channel*>	channels;
	std::string						serverName;
};

#endif