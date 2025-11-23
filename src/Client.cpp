#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Client::Client(int fd) : fD(fd), nickName(""), userName(""), is_autenticated(false), buffer("") {}

Client::~Client() {}

int			Client::get_fD() const {
	return fD;
}

std::string	Client::get_nickName() const {
	return nickName;
}

std::string	Client::get_userName() const {
	return userName;
}

std::string	Client::get_realName() const {
	return realName;
}

std::string&	Client::getWriteBuffer() {
	return writeBuffer;
}

void		Client::setNickName(const std::string& nickname) {
	nickName = nickname;
}

void		Client::setUserName(const std::string& username) {
	userName = username;
}

void		Client::setRealName(const std::string& realname) {
	realName = realname;
}

bool		Client::isRegistered() const {
	if (!userName.empty() && !nickName.empty() && is_autenticated)
		return true;
	return false;
}

bool		Client::isAutenticated() const {
	return is_autenticated;
}

void		Client::appendToBuffer(const std::string& str) {
	buffer += str;
}

std::string&	Client::get_buffer()
{
	return buffer;
}

void	Client::setAuthenticated()
{
	is_autenticated = true;
}

std::string	Client::getHostname() const
{
	struct sockaddr_in	addr;
	socklen_t			addr_len = sizeof(addr);

	if (getpeername(fD, (struct sockaddr*)&addr, &addr_len) == 0)
	{
		char*	ip = inet_ntoa(addr.sin_addr);
		if (ip)
			return std::string(ip);
	}
	return "unknown";
}

const std::map<std::string, Channel*>&	Client::getChannelList() const
{
	return channels;
}

void	Client::appendToWriteBuffer(const std::string& reply)
{
	writeBuffer.append(reply);
}

bool	Client::hasDataToSend()
{
	if (writeBuffer.empty())
		return false;
	return true;
}

void	Client::addChannel(const std::string& channelName, Channel *channel) {
	channels[channelName] = channel;
}

void	Client::removeChannel(const std::string& channelName) {
	channels.erase(channelName);
}