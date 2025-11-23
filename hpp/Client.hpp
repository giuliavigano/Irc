#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <list>
#include <map>
#include <string>
#include "Channel.hpp"

class Server;

class Client {
public:
	Client(int fd);
	~Client();

	int								get_fD() const;
	std::string						get_nickName() const;
	std::string						get_userName() const;
	std::string						get_realName() const;
	std::string&					get_buffer();
	std::string&					getWriteBuffer();
	std::string						getHostname() const;

	const std::map<std::string, Channel*>&	getChannelList() const;

	void		setNickName(const std::string& nickname);
	void		setUserName(const std::string& userName);
	void		setRealName(const std::string& realname);
	void		setAuthenticated();

	bool		isRegistered() const;
	bool		isAutenticated() const;
	
	void		appendToWriteBuffer(const std::string& reply);
	void		appendToBuffer(const std::string& str);
	bool		hasDataToSend();

	void		addChannel(const std::string& channelName, Channel *channel);
	void		removeChannel(const std::string& channelName);
private:
	int								fD;
	std::string						nickName;
	std::string						userName;
	std::string						realName;
	bool							is_autenticated;
	std::map<std::string, Channel*>	channels;
	std::string						buffer;
	std::string						writeBuffer;
};


#endif