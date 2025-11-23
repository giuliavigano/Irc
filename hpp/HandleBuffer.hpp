#ifndef HANDLEBUFFER_HPP
# define HANDLEBUFFER_HPP

# include <string>
# include <vector>

class Server;
class Client;

class HandleBuffer
{
	private:
		static void	parseMessage(Client& client, const std::string& message, Server& server);

		static void	handlePass(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handleNick(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handleUser(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handleMsg(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handlePing(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handleQuit(Client& client, Server& server, const std::vector<std::string>& params);
		static void handleCap(Client& client, Server& server, const std::vector<std::string>& params);
		static void	handleWho(Client& client, Server& server, const std::vector<std::string>& params);

	public:
		static void	lookForCompleteMessage(Client& client, Server& server);
		static std::string	capitalize(const std::string& str);
};

#endif