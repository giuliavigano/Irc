/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mchiaram <mchiaram@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 14:06:52 by mchiaram          #+#    #+#             */
/*   Updated: 2025/12/01 14:06:53 by mchiaram         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Bot.hpp"

Bot::Bot(char** argv) : _socket(-1), _ip(argv[1]), _pass(argv[3]), _nick("Norminette"), _stop(false)
{
	std::stringstream	ss(argv[2]);
	
	ss >> _port;
	if (ss.fail() || (_port < 0 || _port > 65535))
	{
		std::cerr << "PORT: Bad input" << std::endl;
		exit(EXIT_FAILURE);
	}
	_bannedWords.push_back("norminette");
	_bannedWords.push_back("moulinette");
	_bannedWords.push_back("examshell");
	_bannedWords.push_back("minishell");
	_bannedWords.push_back("stupido");
	_bannedWords.push_back("scemo");
}

Bot::~Bot()
{
	if (_socket != -1)
		close(_socket);
}

void	Bot::run()
{
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
	{
		std::cerr << "SOCKET: Creation error" << std::endl;
		return ;
	}

	struct sockaddr_in	serv_addr;
	std::memset(&serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(_port);

	if (inet_pton(AF_INET, _ip.c_str(), &serv_addr.sin_addr) <= 0)
	{
		std::cerr << "Error: Invalid IP address. Use numeric IP (127.0.0.1)" << std::endl;
		return ;
	}

	if (connect(_socket, (struct sockaddr*)& serv_addr, sizeof(serv_addr)) < 0)
	{
		std::cerr << "Connection failed..." << std::endl;
		return ;
	}
	std::cout << "Connected to server!" << std::endl;
	
	sendMessage("PASS " + _pass);
	sendMessage("NICK " + _nick);
	sendMessage("USER bot 0 * :42 Bot");
	sleep(1); //race condition

	char	buffer[512];
	while (!_stop)
	{
		std::memset(buffer, 0, 512);

		int	bytes = recv(_socket, buffer, 511, 0);
		if (bytes <= 0)
			break ;
		
		std::string	rawData(buffer);
		size_t	pos = 0;
		while ((pos = rawData.find("\r\n")) != std::string::npos)
		{
			std::string	line = rawData.substr(0, pos);
			parseCommand(std::string(line));
			rawData.erase(0, pos + 2);
		}
	}
}

void	Bot::sendMessage(std::string message)
{
	message += "\r\n";
	send(_socket, message.c_str(), message.length(), 0);
	std::cout << ">>" << message;
}

void	Bot::parseCommand(std::string buffer)
{
	typedef void		(*CommandHandler)(Bot&, std::vector<std::string>& params);
	static std::map<std::string, CommandHandler>	commandMap;

	if (commandMap.empty())
	{
		commandMap["PRIVMSG"] = &handlePrivmsg;
		commandMap["INVITE"] = &handleInvite;
		commandMap["KICK"] = &handleKick;
		commandMap["JOIN"] = &handleJoin;
		commandMap["353"] = &handleNameRPL;
	}

	std::stringstream	ss(buffer);
	std::vector<std::string>	params;
	std::string					word;
	while (ss >> word)
		params.push_back(word);

	std::map<std::string, CommandHandler>::iterator it = commandMap.find(params[1]);

	if (it != commandMap.end())
	{
		CommandHandler	handler = it->second;
		handler(*this, params);
	}
}

static std::string	lowercase(std::string param)
{
	std::string	lowercaseParam = "";
	for (size_t i = 0; i < param.length(); i ++)
		lowercaseParam += std::tolower(param[i]);
	return lowercaseParam;
}

void	Bot::handlePrivmsg(Bot& bot, std::vector<std::string>& params)
{
	if (params.size() >= 4)
		params[3] = params[3].substr(1);
	for (size_t i = 3; i < params.size(); i++)
	{
		for (size_t j = 0; j < bot._bannedWords.size(); j++)
		{
			if (lowercase(params[i]) == bot._bannedWords[j])
			{
				size_t	targetEnd = params[0].find("!");
				if (targetEnd == std::string::npos)
					return ;
				std::string	target = params[0].substr(1, targetEnd - 1);
				std::string	reason;
				for (size_t x = 3; x < params.size(); x++)
					reason += params[x] + " ";
				reason = reason.substr(0, reason.length() - 1);
				bot.sendMessage("KICK " + params[2] + " " + target + " :Bad word \"" + reason + "\"");
				return;
			}
		}
	}
}

void	Bot::handleInvite(Bot& bot, std::vector<std::string>& params)
{
	if (params.size() >= 4)
	{
		std::string	channel = params[3];
		bot.sendMessage("JOIN " + channel);
	}
}

void	Bot::handleKick(Bot& bot, std::vector<std::string>& params)
{
	std::map<std::string, bool>&	users = bot.getUsersMap(params[2]);
	
	std::map<std::string, bool>::iterator	it = users.find(params[3]);
	if (it != users.end())
		users.erase(it);
	
	if (users.size() == 1)
	{
		bot.sendMessage("PART " + params[2] + " :Empty channel");
		return ;
	}

	bot.checkChannelOps(users, params[2]);
}

void	Bot::handleNameRPL(Bot& bot, std::vector<std::string>& params)
{
	std::map<std::string, bool>&	users = bot.getUsersMap(params[4]);
	
	users.clear();
	for (size_t i = 5; i < params.size(); i++)
	{
		if (params[i][0] == ':')
			params[i] = params[i].substr(1);

		if (params[i][0] == '@')
			users[params[i].substr(1)] = true;
		else
			users[params[i]] = false;
	}
	if (users.size() == 1)
	{
		bot.sendMessage("PART " + params[4] + " :Empty channel");
		return ;
	}
	
	bot.checkChannelOps(users, params[4]);
}

std::map<std::string, bool>&	Bot::getUsersMap(const std::string& channel)
{
	return _channelUsers[channel];
}

void	Bot::checkChannelOps(std::map<std::string, bool>& users, std::string channel)
{
	for (std::map<std::string, bool>::iterator it = users.begin(); it != users.end(); it++)
	{
		if (it->first != _nick && it->second == true)
			return ;
	}
	std::string	target = "";
	if (users.begin()->first == _nick)
		target = (++users.begin())->first;
	else
		target = users.begin()->first;
	sendMessage("MODE " + channel + " +o " + target);
}

void	Bot::handleJoin(Bot& bot, std::vector<std::string>& params)
{
	std::map<std::string, bool>& users = bot.getUsersMap(params[2]);
	size_t	nickEnd = params[0].find("!");

	if (nickEnd == std::string::npos)
		return ;
	std::string	target = params[0].substr(1, nickEnd - 1);
	users[target] = false;
}