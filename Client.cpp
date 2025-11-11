#include "Client.hpp"

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

void		Client::setNickName(const std::string& nickname) {
	nickName = nickname;
}

void		Client::setUserName(const std::string& username) {
	userName = username;
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