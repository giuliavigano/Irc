/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mchiaram <mchiaram@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 14:06:55 by mchiaram          #+#    #+#             */
/*   Updated: 2025/12/01 14:06:56 by mchiaram         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
# define BOT_HPP

# include <iostream>
# include <sstream>
# include <unistd.h>
# include <vector>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <cstring>
# include <map>
# include <cstdlib>
# include <algorithm>

class	Bot
{
	private:
		int													_socket;
		const std::string									_ip;
		const std::string									_pass;
		std::string											_nick;
		bool												_stop;
		int													_port;
		std::vector<std::string>							_bannedWords;
		std::map<std::string, std::map<std::string, bool> >	_channelUsers;

		std::map<std::string, bool>&	getUsersMap(const std::string& channel);
		void	sendMessage(std::string message);
		void	parseCommand(std::string buffer);
		void	checkChannelOps(std::map<std::string, bool>&, std::string);
		static void	handlePrivmsg(Bot&, std::vector<std::string>& params);
		static void	handleInvite(Bot&, std::vector<std::string>& params);
		static void	handleKick(Bot&, std::vector<std::string>& params);
		static void	handleNameRPL(Bot&, std::vector<std::string>& params);
		static void	handleJoin(Bot&, std::vector<std::string>& params);
		
	public:
		Bot(char** argv);
		~Bot();

		void	run();
};

#endif