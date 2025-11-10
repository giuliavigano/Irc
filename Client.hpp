#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <list>
#include <string>

class Server;

class Client {
public:
	Client(int fd);
	~Client();
private:
	int			fD;
	std::string	nickName;
	std::string	userName;
	std::string	realName;
	bool					is_autenticated;
	std::list<std::string>	channels;
	std::string				buffer_text;

};

#endif