#ifndef CHANNEL_HPP
# define CHANNEL_HPP
#include <list>
#include <string>
#include <iostream>

class Client;

class Channel {
public:
	
private:
	std::string			name;
	std::list<Client>	clients;
};

#endif