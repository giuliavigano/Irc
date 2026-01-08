/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: giuliaviga <giuliaviga@student.42.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 14:10:22 by gvigano           #+#    #+#             */
/*   Updated: 2025/12/08 12:23:39 by giuliaviga       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//	Parametric constructor 
Client::Client(int fd) : fD(fd), nickName(""), userName(""), is_autenticated(false), buffer("") {}

//	Default destructor
Client::~Client() {}

//	Returns the file descriptor of the client
int			Client::get_fD() const {
	return fD;
}

//	Returns the nickname of the client
std::string	Client::get_nickName() const {
	return nickName;
}

//	Returns the username of the client
std::string	Client::get_userName() const {
	return userName;
}

//	Returns the realname of the client
std::string	Client::get_realName() const {
	return realName;
}

//	Returns the writebuffer of this client
std::string&	Client::getWriteBuffer() {
	return writeBuffer;
}

//	Sets the nickname of the client
void		Client::setNickName(const std::string& nickname) {
	nickName = nickname;
}

//	Sets the username of the client
void		Client::setUserName(const std::string& username) {
	userName = username;
}

//	Sets the realname of the client
void		Client::setRealName(const std::string& realname) {
	realName = realname;
}

//	Returns whether the client is registered to the server or not
bool		Client::isRegistered() const {
	if (!userName.empty() && !nickName.empty() && is_autenticated)
		return true;
	return false;
}

//	Returns whether the client is autenticated by the server or not
bool		Client::isAutenticated() const {
	return is_autenticated;
}

//	Fills the READ buffer for the messages client to server
void		Client::appendToBuffer(const std::string& str) {
	buffer += str;
}

//	Returns the READ buffer containing messages from the client
std::string&	Client::get_buffer()
{
	return buffer;
}

//	Sets the autentication flag for this client to true
void	Client::setAuthenticated()
{
	is_autenticated = true;
}

//	Returns the hostname/IP address of the client
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

//	Returns the channels list of this client
const std::map<std::string, Channel*>&	Client::getChannelList() const
{
	return channels;
}

//	Fills the WRITE buffer for the message the server will send to this client
void	Client::appendToWriteBuffer(const std::string& reply)
{
	writeBuffer.append(reply);
}

//	Returns whether the WRITE buffer is empty or not
bool	Client::hasDataToSend()
{
	if (writeBuffer.empty())
		return false;
	return true;
}

//	Adds a channel to the channel list of the client
void	Client::addChannel(const std::string& channelName, Channel *channel) {
	channels[channelName] = channel;
}

//	Removes a channel from the channel list of the client
void	Client::removeChannel(const std::string& channelName) {
	channels.erase(channelName);
}