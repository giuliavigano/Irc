/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HandleBuffer.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mchiaram <mchiaram@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 14:07:24 by mchiaram          #+#    #+#             */
/*   Updated: 2025/12/01 14:07:25 by mchiaram         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HandleBuffer.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include <iostream>
#include <map>
#include <sstream>

static std::string	ft_uppercase(std::string str)
{
	std::string	upperCommand;
	for (size_t i = 0; i < str.size(); i++)
		upperCommand += std::toupper(str[i]);
	return upperCommand;
}

//Reads the buffer and looks for a complete message.
void	HandleBuffer::lookForCompleteMessage(Client& client, Server& server)
{
	std::string&	buffer = client.get_buffer();
	size_t	pos;

	int		client_fd = client.get_fD();
	
	while ((pos = buffer.find("\r\n")) != std::string::npos)
	{
		std::string message = buffer.substr(0, pos);
		parseMessage(client, message, server);

		std::map<int, Client*> clients = server.getClientsMap();
		if (clients.find(client_fd) == clients.end())
			return ;

		buffer = buffer.substr(pos + 2); //Buffer update: removed any complete message and left any incomplete one.
	}
}

void	HandleBuffer::parseMessage(Client& client, const std::string& message, Server& server)
{
	//Declaration of handler function type
	typedef void	(*CommandHandler)(Client&, Server&, const std::vector<std::string>&);

	//static map<string, func> created once first time entering parseMessage function
	static std::map<std::string, CommandHandler>	commandMap;
	if (commandMap.empty())
	{
		commandMap["PASS"] = &handlePass;
		commandMap["NICK"] = &handleNick;
		commandMap["USER"] = &handleUser;
		commandMap["PRIVMSG"] = &handleMsg;
		commandMap["PING"] = &handlePing;
		commandMap["QUIT"] = &handleQuit;
		commandMap["CAP"] = &handleCap;
		commandMap["WHO"] = &handleWho;
		commandMap["INVITE"] = &Channel::handleInvite;
		commandMap["KICK"] = &Channel::handleKick;
		commandMap["TOPIC"] = &Channel::handleTopic;
		commandMap["JOIN"] = &Channel::handleJoin;
		commandMap["MODE"] = &Channel::handleMode;
		commandMap["PART"] = &Channel::handlePart;
	}

	//Checking the entire message string using stringstream
	std::stringstream	ss(message);
	std::string			command;
	ss >> command; //extraction of first word, should be the command

	command = ft_uppercase(command);
	//Extraction of every parameter following the command
	std::vector<std::string>	params;
	std::string					param;
	while (ss >> param)
		params.push_back(param);

	//search commandMap for command key
	std::map<std::string, CommandHandler>::iterator it = commandMap.find(command);

	if (it != commandMap.end())
	{
		CommandHandler	handler = it->second;
		handler(client, server, params);
	}
	else
		server.sendReply(ERR_UNKNOWNCOMMAND, command + ": unkown command", client);
}

//Checks if client is already authenticated, if not authenticates it checking the password sent
//Sends an error to the client if something is off
void	HandleBuffer::handlePass(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (client.isAutenticated())
	{
		server.sendReply(ERR_ALREADYREGISTERED, "PASS: Already authenticated", client);
		return ;
	}
	if (params.empty())
	{
		server.sendReply(ERR_NEEDMOREPARAMS, "PASS: Not enough parameters", client);
		return ;
	}

	if (params[0] == server.getPsw())
		client.setAuthenticated();
	else
		server.sendReply(ERR_PASSWDMISMATCH, "PASS: Wrong password", client);
}

std::string	HandleBuffer::capitalize(const std::string& str)
{
	if (str.empty())
		return str;

	std::string	capitalized = str;
	capitalized[0] = std::toupper(capitalized[0]);

	for (size_t i = 1; i < capitalized.length(); i++)
		capitalized[i] = std::tolower(capitalized[i]);
	
	return capitalized;
}

static bool	validNickname(const std::string& nickname, Server& server, Client& client)
{
	if (nickname.empty())
	{
		server.sendReply(ERR_NONICKNAMEGIVEN, "NICK: Nickname not given", client);
		return false;
	}

	if (isdigit(nickname[0]) || nickname[0] == '-')
	{
		server.sendReply(ERR_ERRONEUSNICKNAME, "NICK: bad nickname. First character can't be a digit or '-'", client);
		return false;
	}

	for (size_t i = 0; i < nickname.length(); i++)
	{
		char	c = nickname[i];
		if (isalnum(c) || c == '-' ||
			(c >= '[' && c <= '`') ||
			(c >= '{' && c <= '}'))
			continue ;

		server.sendReply(ERR_ERRONEUSNICKNAME, "NICK: bad nickname", client);
		return false;
	}
	return true;
}

void	HandleBuffer::handleNick(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (!client.isAutenticated())
	{
		server.sendReply(ERR_NOTREGISTERED, "NICK: Not authorized, password missing", client);
		return ;
	}
	if (params.size() == 0)
	{
		server.sendReply(ERR_NONICKNAMEGIVEN, "NICK: Nickname not given", client);
		return ;
	}
	std::string	nickname = capitalize(params[0]);

	if (!validNickname(nickname, server, client))
		return ;

	if (nickname.length() > 30)
		nickname = nickname.substr(0, 30);

	std::map<int, Client*>	clientsMap = server.getClientsMap();

	for (std::map<int, Client *>::iterator it = clientsMap.begin(); it != clientsMap.end(); it++)
	{
		if (nickname == it->second->get_nickName())
		{
			server.sendReply(ERR_NICKNAMEINUSE, "NICK: Nickname already in use", client);
			return ;
		}
	}
	
	if (!client.get_userName().empty() && client.get_nickName().empty())
	{
		client.setNickName(nickname);
		std::string	welcomeMessage = ":Welcome to the IRC Network " + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname();
		server.sendReply(RPL_WELCOME, welcomeMessage, client);
		return ;
	}
	else if (!client.get_userName().empty())
	{
		std::map<std::string, Channel*>	channelsMap = client.getChannelList();
		std::string	message = ":" + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname() + " NICK :" + capitalize(nickname) + "\r\n";

		for (std::map<std::string, Channel*>::iterator it = channelsMap.begin(); it != channelsMap.end(); it++)
			it->second->broadcast(message, &client);
		client.appendToWriteBuffer(message);
	}
	client.setNickName(nickname);
}

void	HandleBuffer::handleUser(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (!client.isAutenticated())
	{
		server.sendReply(ERR_NOTREGISTERED, "USER: Not authorized, password missing", client);
		return ;
	}
	if (client.isRegistered())
	{
		server.sendReply(ERR_ALREADYREGISTERED, "USER: Unauthorized command (already registered)", client);
		return ;
	}
	if (params.size() < 4)
	{
		server.sendReply(ERR_NEEDMOREPARAMS, "USER: Not enough parameters: USER <username> <0> <*> :<realname>", client);
		return ;
	}
	if (!client.get_userName().empty())
	{
		server.sendReply(ERR_ALREADYREGISTERED, "USER: Username already set", client);
		return ;
	}

	std::string	username = params[0];
	std::string	realname = params[3];

	for (size_t i = 0; i < username.length(); i++)
	{
		if (username[i] == '@' || username[i] == ' ' || 
			username[i] == '\r' || username[i] == '\n')
			{
				server.sendReply(ERR_ERRONEUSNICKNAME, "USER: bad username", client);
				return ;
			}
	}

	if (!realname.empty() && realname[0] == ':')
	{
		realname = realname.substr(1);
		for (size_t i = 4; i < params.size(); i++)
			realname += " " + params[i];
	}

	for (size_t i = 0; i < realname.length(); i++)
	{
		char	c = realname[i];
		if (c < 32 || c > 126)
		{
			server.sendReply(ERR_ERRONEUSNICKNAME, "USER: bad realname", client);
			return ;
		}
	}
	
	client.setUserName(username);
	client.setRealName(realname);

	if (!client.get_nickName().empty())
	{
		std::string	welcomeMessage = ":Welcome to the IRC Network " + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname();
		server.sendReply(RPL_WELCOME, welcomeMessage, client);
	}
}

static Client*	findClient(const std::string& target, const std::map<int, Client *>& clientsMap)
{
	for (std::map<int, Client*>::const_iterator it = clientsMap.begin(); it != clientsMap.end(); it++)
	{
		if (it->second->get_nickName() == target)
			return it->second;
	}
	return NULL;
}

static Channel*	findChannel(const std::string& target, const std::map<std::string, Channel*>& channelsMap)
{
	for (std::map<std::string, Channel*>::const_iterator it = channelsMap.begin(); it != channelsMap.end(); it++)
	{
		if (it->first == target)
			return it->second;
	}
	return NULL;
}

void	HandleBuffer::handleMsg(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (!client.isRegistered())
	{
		server.sendReply(ERR_NOTREGISTERED, "PRIVMSG: Not authorized, register first", client);
		return ;
	}
	if (params.size() < 2)
	{
		server.sendReply(ERR_NEEDMOREPARAMS, "PRIVMSG: Not enough parameters", client);
		return ;
	}

	std::string	target = params[0];
	std::string	message = params[1];

	if (message[0] == ':')
	{
		message = message.substr(1);
		for (size_t i = 2; i < params.size(); i++)
			message += " " + params[i];
	}

	if (target[0] == '#')
	{
		Channel*	channel = findChannel(target, server.getChannelsMap());
		if (!channel)
		{
			server.sendReply(ERR_NOSUCHCHANNEL, "PRIVMSG: Channel: " + target + " doesn't exist", client);
			return ;
		}
		if (!channel->isClientInChannel(client.get_fD()))
		{
			server.sendReply(ERR_CANNOTSENDTOCHAN, "PRIVMSG: User not in channel " + target, client);
			return ;
		}
		std::string	broadcastMsg = ":" + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
		channel->broadcast(broadcastMsg, &client);
		return ;
	}

	Client*	targetClient = findClient(capitalize(target), server.getClientsMap());
	if (!targetClient)
	{
		server.sendReply(ERR_NOSUCHNICK, "PRIVMSG: User " + target + " doesn't exist", client);
		return ;
	}
	std::string	sendMsg = ":" + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
	targetClient->appendToWriteBuffer(sendMsg);
}

void	HandleBuffer::handlePing(Client& client, Server& server, const std::vector<std::string>& params)
{
	(void)params;
	client.appendToWriteBuffer("PONG :" + server.getServerName());
}

void	HandleBuffer::handleQuit(Client& client, Server& server, const std::vector<std::string>& params)
{
	std::map<std::string, Channel *>	clientChannels = client.getChannelList();
	std::string quitMsg = ":" + client.get_nickName() + "!" + client.get_userName() + "@" + client.getHostname() + " QUIT ";
	
	if (!params.empty())
	{
		for (size_t i = 0; i < params.size(); i++)
		{
			if (i == 0)
				quitMsg += ":" + params[i];
			else
				quitMsg += " " + params[i];
		}
	}
	else
		quitMsg += ":Client Quit";
	quitMsg += "\r\n";
	for (std::map<std::string, Channel *>::const_iterator it = clientChannels.begin(); it != clientChannels.end(); it++)
	{
		it->second->broadcast(quitMsg, &client);
		it->second->removeClient(&client, &server);
		server.ifRemoveChannel(it->second->getName());
	}

	server.eraseClient(&client);
}

void	HandleBuffer::handleCap(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (params.empty())
        return;

	if (params[0] == "LS")
	{
		std::string response = ":" + server.getServerName() + " CAP * LS :\r\n";
        client.appendToWriteBuffer(response);
	}
}

void	HandleBuffer::handleWho(Client& client, Server& server, const std::vector<std::string>& params)
{
	if (params.empty())
	{
		server.sendReply(ERR_NEEDMOREPARAMS, "WHO: Not enough arguments", client);
		return ;
	}

	std::string	target = params[0];
	if (target[0] == '#')
	{
		std::map<std::string, Channel*>& channels = server.getChannelsMap();
		
		if (channels.find(target) != channels.end())
		{
			Channel* channel = channels[target];
			std::map<int, Client*>	members = channel->getClientMap();

			for (std::map<int, Client*>::iterator it = members.begin(); it != members.end(); it++)
			{
				Client*	member = it->second;
				std::string	flags = "H";

				if (channel->isOperator(member->get_fD()))
					flags += "@";
				
				std::string	message = target + " " +
										member->get_userName() + " " +
										member->getHostname() + " " +
										server.getServerName() + " " +
										member->get_nickName() + " " +
										flags + " :0" +
										member->get_realName();

				std::cout << message << std::endl;
				server.sendReply(RPL_WHOREPLY, message, client);
			}
		}
	}
	server.sendReply(RPL_ENDOFWHO, target + " : end of who list", client);
}