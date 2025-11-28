#include "Channel.hpp"
#include "Replies.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "HandleBuffer.hpp"

Channel::Channel(const std::string& name) : name(name), topic(""), user_limit(0), topic_setter_info(""), topic_time(0), key("") {}

Channel::~Channel() {}

///======================================== UTILS ===========================================
static int	getFdByNickname(const std::map<int, Client *>& clientsMap, const std::string& nickname)
{
	for (std::map<int, Client *>::const_iterator it = clientsMap.begin(); it != clientsMap.end(); it++)
	{
		if (it->second->get_nickName() == nickname)
			return it->first;
	}
	return 0;
}

static std::string	lowercase(const std::string& str) {
	std::string 	lowercase;
	for (size_t i = 0; i < str.size(); i++) {
		lowercase += std::tolower(str[i]);
	}
	return lowercase;
}

static std::vector<std::string>	splitByComma(const std::string& params) {
	std::vector<std::string>	result;
	size_t	start = 0;
	size_t	end = params.find(',');
	while (end != std::string::npos) {
		result.push_back((params.substr(start, end - start)));
		start = end + 1;
		end = params.find(',', start);
	}
	result.push_back(params.substr(start));
	return result;
}

void			Channel::addClient(Client* client) {
	if (clients.find(client->get_fD()) == clients.end()) {
		clients[client->get_fD()] = client;
		client->addChannel(name, this);
	}
}

void			Channel::removeClient(Client* client, Server* server) {
	if (clients.find(client->get_fD()) == clients.end())
		return;
	clients.erase(client->get_fD());
	client->removeChannel(name);
	operators.erase(client->get_fD());
	if (!hasOperators() && !clients.empty())
		makeOperator(clients.begin()->first);
	if (server)
		sendOperatorUpdate(*server);
}

bool			Channel::isClientInChannel(int client_fd) const {
	if (clients.find(client_fd) != clients.end())
		return true;
	return false;
}

bool			Channel::isClientInChannel(std::string name) const {
	for(std::map<int, Client*>::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		if (i->second->get_nickName() == name)
			return true;
	}
	return false;
}

bool			Channel::isAValidChannel(std::string name) {
	if (name.size() < 2 || name.size() > 50)
		return false;
	if (name[0] != '#')
		return false;
	for (size_t i = 0; i < name.size(); ++i) {
		char c = name[i];
		if (c == ' ' || c == ',' || c == ':' || c == '\x07')
			return false;
		if ((c >= 0 && c <= 31) || c == 127)
			return false;
	}
	return true;
}

bool			Channel::isRightPassword(std::string client_passw) const {
	return (client_passw == key);
}

std::string		Channel::getName() const {
	return	name;
}

std::string		Channel::getKey() const {
	return key;
}

std::map<int, Client*>&	Channel::getClientMap() {
	return clients;
}

void			Channel::setKey(const std::string& password) {
	key = password;
}

bool			Channel::isEmpty() const {
	return clients.empty();
}

bool			Channel::isOperator(int client_fd) const {
	return (operators.count(client_fd) > 0);
}

void			Channel::makeOperator(int client_fd) {
	operators.insert(client_fd);
}

void			Channel::removeOperator(int client_fd) {
	operators.erase(client_fd);
}

void			Channel::broadcast(const std::string& message, Client* excludeClient) {
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (excludeClient == NULL || it->second->get_fD() != excludeClient->get_fD()) {
            it->second->appendToWriteBuffer(message);
        }
    }
}

bool	Channel::hasOperators()
{
	if (operators.empty())
		return false;
	return true;
}

void	Channel::sendOperatorUpdate(Server& server)
{
	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); it++)
		sendClientInfo(*it->second, server);
}

// ======================================= JOIN =========================================
bool			Channel::isInvited(int client_fd) const {	
	return (invited.count(client_fd) > 0);
}

void			Channel::sendJoinChange(Client& client) {
	std::string message = ":" + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname() + " JOIN " + name + "\r\n";
	broadcast(message);
}

void			Channel::sendClientInfo(Client& client, Server& server) {
	if (!topic.empty()) {
		server.sendReply(RPL_TOPIC, name + " :" + topic, client);
	} else {
		server.sendReply(RPL_NOTOPIC, name + " :No topic is set", client);
	}
	std::string		userList = "";
	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
		if (it != clients.begin())
			userList += " ";
		if (isOperator(it->first))
			userList += "@";
		userList += it->second->get_nickName();
	}
	server.sendReply(RPL_NAMREPLY, "= " + name + " :" + userList, client);
	server.sendReply(RPL_ENDOFNAMES, name + " :End of /NAMES list", client);
}

void			Channel::handleJoin(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.empty()) {
		server.sendReply(ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters", client);
		return;
	}
	std::vector<std::string>	channelsParam = splitByComma(params[0]);
	std::vector<std::string>	keyParam;
	size_t						j = 0;
	if (params.size() >= 2) 
		keyParam = splitByComma(params[1]);
	for (size_t i = 0; i < channelsParam.size(); i++) {
		std::string		nameChannel = lowercase(channelsParam[i]);
		std::map<std::string, Channel*>&	channels = server.getChannelsMap();
		if (channels.find(nameChannel) == channels.end()) {
			if (!Channel::isAValidChannel(nameChannel)) {
				server.sendReply(ERR_NOSUCHCHANNEL, nameChannel + " :No such channel", client);
				continue;
			} else {
				Channel*	channel = new Channel(nameChannel);
				channels[nameChannel] = channel;
				channel->addClient(&client);
				channel->makeOperator(client.get_fD());
				channel->sendJoinChange(client);
				channel->sendClientInfo(client, server);
			}
		} else {
			Channel*	channel = channels[nameChannel];
			if (channel->isClientInChannel(client.get_fD())) {
				server.sendReply(ERR_USERONCHANNEL, client.get_userName() + " " + nameChannel + " :is already on channel", client);
				continue;
			}
			if (channel->isInvited(client.get_fD())) {
				channel->invited.erase(client.get_fD());
			} else {
				if (channel->hasMode('i')) {
					server.sendReply(ERR_INVITEONLYCHAN, nameChannel + " :Cannot join channel", client);
					continue;
				}
				if (channel->hasMode('l')) {
					if (channel->clients.size() >= (size_t)channel->user_limit) {
						server.sendReply(ERR_CHANNELISFULL, nameChannel + " :Cannot join channel", client);
						continue;
					}
				}
				if (channel->hasMode('k')) {
					if (params.size() != 2 || !channel->isRightPassword(keyParam[j++])) {
						server.sendReply(ERR_BADCHANNELKEY, nameChannel + " :Cannot join channel", client);
						continue;
					}
				}
			}
			channel->addClient(&client);
			channel->sendJoinChange(client);
			channel->sendClientInfo(client, server);
		}
	}
}


// ======================================= INVITE =========================================
bool			Channel::canInviteClient(int client_fd) const {
	if (hasMode('i'))
		return (isOperator(client_fd));
	return true;
}

void			Channel::inviteClient(Client* invitedClient, std::string nickname, int client_fd) {
	invited.insert(invitedClient->get_fD());
	std::string		message = ":" + clients[client_fd]->get_nickName() + "!" + clients[client_fd]->get_userName() + "@" + clients[client_fd]->getHostname() + " INVITE " + nickname + " " + name + "\r\n";
	invitedClient->appendToWriteBuffer(message);
}

void			Channel::handleInvite(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.size() < 2) {
		server.sendReply(ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters", client);
		return;
	}
	std::string		channelName = lowercase(params[1]);
	std::map<std::string, Channel*>&	channels = server.getChannelsMap();
	if (channels.find(channelName) == channels.end()) {
		server.sendReply(ERR_NOSUCHCHANNEL, channelName + " :No such channel", client);
		return;
	}
	Channel*	channel = channels[channelName];
	std::string	clientNick = HandleBuffer::capitalize(params[0]);
	if(!channel->isClientInChannel(client.get_nickName())) {
		server.sendReply(ERR_NOTONCHANNEL, channelName + " :You're not on that channel", client);
		return;
	}
	if (channel->isClientInChannel(clientNick)) {
		server.sendReply(ERR_USERONCHANNEL, clientNick + " " + channelName + " :is already on channel", client);
		return;
	}
	if (!channel->canInviteClient(client.get_fD())) {
		server.sendReply(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator", client);
		return;
	}
	std::map<int, Client*>	clients = server.getClientsMap();
	Client*	targetClient = NULL;
	for (std::map<int, Client*>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (clients[i->first]->get_nickName() == clientNick) {
			targetClient = i->second;
		}
	}
	if (!targetClient) {
		server.sendReply(ERR_NOSUCHNICK, clientNick + " :No such nick/channel", client);
		return;
	}
	else {
		server.sendReply(RPL_INVITING, clientNick + " " + channelName, client);
		channel->inviteClient(targetClient, clientNick, client.get_fD());
	}
}

//========================================= KICK ==========================================
bool			Channel::canKickClient(int kicker_fd) const {
	if (isOperator(kicker_fd))
		return true;
	return false;
}

void			Channel::kickClient(std::string clientOut, int kicker_fd, std::string reason) {
	sendKickChange(clientOut, kicker_fd, reason);
	for (std::map<int, Client*>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (i->second->get_nickName() == clientOut) {
			removeClient(i->second);
			return;
		}
	}
}

void			Channel::sendKickChange(std::string nameOut, int kicker_fd, std::string reason) {
	std::string			message;
	if (!reason.empty())
		message = ":" + clients[kicker_fd]->get_nickName() + "!" + clients[kicker_fd]->get_userName() + "@" + clients[kicker_fd]->getHostname() + " KICK " + name +  " " + nameOut + " :" + reason + "\r\n";
	else
		message = ":" + clients[kicker_fd]->get_nickName() + "!" + clients[kicker_fd]->get_userName() + "@" + clients[kicker_fd]->getHostname() + " KICK " + name +  " " + nameOut + "\r\n";
	broadcast(message);
}

void			Channel::handleKick(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.size() < 2 ) {
		server.sendReply(ERR_NEEDMOREPARAMS, "KICK :Not enough parameters", client);
		return;
	}
	size_t			i = 0;
	if (params[i][0] != '#')
		i++;
	std::string		channelName = lowercase(params[i++]);
	std::map<std::string, Channel*>&	channels = server.getChannelsMap();
	if (channels.find(channelName) == channels.end()) {
		server.sendReply(ERR_NOSUCHCHANNEL, channelName + " :No such channel", client);
		return;
	}
	Channel*	channel = channels[channelName];
	if (i >= params.size()) {
		server.sendReply(ERR_NOSUCHCHANNEL, " :Bad format, NICK <channel> <nick>", client);
		return ;
	}
	std::string	targetNick;
	if (params[i][0] == ':')
		targetNick = HandleBuffer::capitalize(params[i++].substr(1));
	else
		targetNick = HandleBuffer::capitalize(params[i++]);
	if (!channel->isClientInChannel(client.get_fD())) {
		server.sendReply(ERR_NOTONCHANNEL, channelName + " :You're not on that channel", client);
		return;
	}
	if (!channel->isClientInChannel(targetNick) ) {
		server.sendReply(ERR_USERNOTINCHANNEL, targetNick + " " + channelName + " :They aren't on that channel", client);
		return;
	}
	if (!channel->canKickClient(client.get_fD())) {
		server.sendReply(ERR_CHANOPRIVSNEEDED, channelName + " :You are not channel operator", client);
		return;
	}
	if (params.size() >= i + 1) {
		std::string		reason = params[i];
		if (!reason.empty() && reason[0] == ':')
			reason = reason.substr(1);
		for (size_t j = i + 1; j < params.size(); j++)
			reason +=  " " + params[j];
		channel->kickClient(targetNick, client.get_fD(), reason);
	} else
		channel->kickClient(targetNick, client.get_fD(), "");
	server.ifRemoveChannel(channelName);
}


//======================================== TOPIC ===========================================
std::string		Channel::getTopic() const {
	return topic;
}

bool			Channel::canChangeTopic(int client_fd) const {
	if (modes.count('t') > 0) { 
		return (isOperator(client_fd));
	}
	return true;
}

void			Channel::setTopic(Client client, std::string newTopic) {
	topic = newTopic;
	topic_setter_info = client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname();
	topic_time = time(NULL);
	sendTopicChange();
}

void			Channel::sendTopicChange() {
	std::string		message = ":" + topic_setter_info + " TOPIC " + name + " :" + topic + "\r\n";
	broadcast(message);
}

void			Channel::handleTopic(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.empty()) {
		server.sendReply(ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters", client);
		return;
	}
	std::string		channelName = lowercase(params[0]);
	std::map<std::string, Channel*>&	channels = server.getChannelsMap();
	if (channels.find(channelName) == channels.end()) {
		server.sendReply(ERR_NOSUCHCHANNEL, channelName + " :No such channel", client);
		return;
	}
	Channel*	channel = channels[channelName];
	if (!channel->isClientInChannel(client.get_fD())) {
		server.sendReply(ERR_NOTONCHANNEL, channelName + " :You're not on that channel", client);
		return;
	}
	if (params.size() == 1) {
		std::string		topic = channel->getTopic();
		if (topic.empty())
			server.sendReply(RPL_NOTOPIC, channelName + " :No topic is set", client);
		else
		{
			std::stringstream	ss;
			server.sendReply(RPL_TOPIC, channelName + " :" + topic, client);
			ss << channel->topic_time;
			std::string	timeStr = ss.str();
			server.sendReply(RPL_TOPICWHOTIME, channelName + " " + channel->topic_setter_info + " " + timeStr, client);
		}
		return;
	}
	if (!channel->canChangeTopic(client.get_fD())) {
		server.sendReply(ERR_CHANOPRIVSNEEDED, channelName+ " :You're not channel operator", client);
		return;
	}
	if (params.size() >= 2) {
		std::string newTopic = params[1];
		if (!newTopic.empty() && newTopic[0] == ':')
			newTopic = newTopic.substr(1);
		for (size_t i = 2; i < params.size(); i++)
			newTopic += " " + params[i];
		channel->setTopic(client, newTopic);
	} else
		channel->setTopic(client, "");
}


//========================================== MODE =============================================
bool			Channel::hasMode(char mode) const {
	return (modes.count(mode) > 0);
}

bool			Channel::canChangeModes(int client_fd) const {
	return (isOperator(client_fd));
}

void			Channel::setMode(const char mode, int client, std::string param) {
	modes.insert(mode);
	sendModeChange(mode, '+', client, param);
}


void			Channel::removeMode(const char mode, int client) {
	if (modes.erase(mode) > 0)
		sendModeChange(mode, '-', client);
}

void			Channel::sendModeChange(const char mode, char sign, int setter_fd, std::string param) {
	std::string		message = ":" + clients[setter_fd]->get_nickName() + "!" + clients[setter_fd]->get_userName() + "@" + clients[setter_fd]->getHostname() + " MODE " + name + " " + sign + mode;
	if (!param.empty())
		message += " " + param;
	message += "\r\n";
	broadcast(message);
}

static bool		modeCheckParams(const std::vector<std::string>& params)
{
	std::string	flags = params[1];
	size_t		nParams = 0;

	for (size_t i = 0; i < flags.size(); i++)
	{
		if (flags[i] == '+')
		{
			while (i < flags.size() && flags[i] != '-')
			{
				if (flags[i] == 'k' || flags[i] == 'o' || flags[i] == 'l')
					nParams++;
				i++;
			}
		}
		if (flags[i] == '-')
		{
			while (i < flags.size() && flags[i] != '+')
			{
				if (flags[i] == 'o')
					nParams++;
				i++;
			}
			if (flags[i] == '+')
				i--;
		}
	}
	
	if (nParams != (params.size() - 2))
		return false;
	return true;
}

void			Channel::handleMode(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.size() < 1) {
		server.sendReply(ERR_NEEDMOREPARAMS, "MODE :Not enough parameters", client);
		return;
	}
	std::string		channelName = lowercase(params[0]);
	std::map<std::string, Channel*>&	channels = server.getChannelsMap();
	if (channels.find(channelName) == channels.end()) {
		server.sendReply(ERR_NOSUCHCHANNEL, channelName + " :No such channel", client);
		return;
	}
	Channel*	channel = channels[channelName];
	if (params.size() == 1) {
		std::string	modeString = "+";
		std::string	modeParams = "";
		for (std::set<char>::iterator it = channel->modes.begin(); it != channel->modes.end(); ++it) {
			modeString += *it;
			if (*it == 'k') {
				modeParams += " " + channel->getKey();
			} else if (*it == 'l') {
				std::ostringstream	oss;
				oss << channel->user_limit;
				modeParams += " " + oss.str();
			}
		}
		server.sendReply(RPL_CHANNELMODEIS, channelName + " " + modeString + modeParams, client);
		return;
	}
	if (!channel->isClientInChannel(client.get_fD())) {
		server.sendReply(ERR_NOTONCHANNEL, channelName + " :You're not on that channel", client);
		return;
	}
	if (!channel->canChangeModes(client.get_fD())) {
		server.sendReply(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator", client);
		return;
	}

	//Check if there are enough params for each requiring mode
	if (!modeCheckParams(params))
	{
		server.sendReply(ERR_NEEDMOREPARAMS, "MODE: Not enough params", client);
		return ;
	}

	size_t		checkedParams = 2;
	bool		add = true;

	for (size_t i = 0; i < params[1].size(); i++)
	{
		char	c = params[1][i];
		
		switch (c)
		{
			case '+':
				add = true;
				break ;
			case '-':
				add = false;
				break ;
			case 'i':
				if (add)
					channel->setMode(c, client.get_fD());
				else
					channel->removeMode(c, client.get_fD());
				break;
			case 't':
				if (add)
					channel->setMode(c, client.get_fD());
				else
					channel->removeMode(c, client.get_fD());
				break ;
			case 'k':
				if (add)
				{
					std::string psw = params[checkedParams++];
					if (!channel->hasMode(c))
					{
						channel->setKey(psw);
						channel->setMode(c, client.get_fD(), channel->key);
					}
				}
				else
				{
					channel->setKey("");
					channel->removeMode(c, client.get_fD());
				}
				break ;
			case 'o':
			{
				std::string	nickTarget = HandleBuffer::capitalize(params[checkedParams++]);
				int			targetFd = getFdByNickname(server.getClientsMap(), nickTarget);
				char		sign = '+';

				if (!channel->isClientInChannel(targetFd)) 
				{
					server.sendReply(ERR_USERNOTINCHANNEL, nickTarget + " " + channelName + " :They aren't on that channel", client);
					break ;
				}
				if (add && !channel->isOperator(targetFd))
					channel->makeOperator(targetFd);
				else if (!add)
				{
					if (channel->operators.size() == 1 && channel->isOperator(targetFd)) {
						server.sendReply(ERR_CHANOPRIVSNEEDED, channelName + " :cannot deop the last operator", client);
						break;
					}
					channel->removeOperator(targetFd);
					sign = '-';
				}
				channel->sendOperatorUpdate(server);
				channel->sendModeChange(c, sign, client.get_fD(), nickTarget);
				break ;
			}
			case 'l':
			{
				if (add)
				{
					std::string			strLimit = params[checkedParams++];
					std::stringstream	ss(strLimit);
					int					limit;

					ss >> limit;
					if (ss.fail() || limit <= 0 || limit >= 1000)
					{
						server.sendReply(ERR_INVALIDMODEPARAM, strLimit + ": invalid limit, must be > 0 and < 1000", client);
						break ;
					}
					channel->user_limit = limit;
					channel->setMode(c, client.get_fD(), strLimit);
				}
				else
				{
					channel->user_limit = 0;
					channel->removeMode(c, client.get_fD());
				}
				break ;
			}
			default:
				server.sendReply(ERR_UNKNOWNMODE, client.get_nickName() + " " + c + " :Is unknown mode char to me", client);
				break;
		}
	}
}


// ===================================== PART ========================================
void			Channel::partClient(Client* client, Server* server, std::string reason) {
	sendPartChange(client->get_fD(), reason);
	removeClient(client, server);
}

void			Channel::sendPartChange(int client_fd, std::string reason) {
	std::string		message = ":" + clients[client_fd]->get_nickName() + "!" + clients[client_fd]->get_userName() + "@" + clients[client_fd]->getHostname() + " PART " + name;
	if (!reason.empty())
		message += " :" + reason;
	message += "\r\n";
	broadcast(message);
}

void			Channel::handlePart(Client& client, Server& server, const std::vector<std::string>& params) {
	if (!client.isAutenticated() || !client.isRegistered()) {
		server.sendReply(ERR_NOTREGISTERED, " :You have not registered", client);
		return;
	}
	if (params.empty()) {
		server.sendReply(ERR_NEEDMOREPARAMS, "PART :Not enough parameters", client);
		return;
	}
	std::string		channelName = lowercase(params[0]);
	std::map<std::string, Channel*>&	channels = server.getChannelsMap();
	if (channels.find(channelName) == channels.end()) {
		server.sendReply(ERR_NOSUCHCHANNEL, channelName + " :No such channel", client);
		return;
	}
	Channel*	channel = channels[channelName];
	if (!channel->isClientInChannel(client.get_fD())) {
		server.sendReply(ERR_NOTONCHANNEL, channelName + " :You're not on that channel", client);
		return;
	}
	if (params.size() >= 2) {
		std::string		reason = params[1];
		if (!reason.empty() && reason[0] == ':')
			reason = reason.substr(1);
		for (size_t i = 2; i < params.size(); i++)
			reason += " " + params[i];
		channel->partClient(&client, &server, reason);
	} else
		channel->partClient(&client, &server, "");
	server.ifRemoveChannel(channelName);
}
