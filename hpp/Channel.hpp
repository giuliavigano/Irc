#ifndef CHANNEL_HPP
# define CHANNEL_HPP
#include <map>
#include <set>
#include <ctime>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <sys/socket.h>

class Client;
class Server;

class Channel {
public:
	Channel(const std::string& name);
	~Channel();

	// UTILS:
	void		addClient(Client* client);
	void		removeClient(Client* client, Server* server = NULL);
	void		makeOperator(int client_fd);
	void		removeOperator(int client_fd);
	void		broadcast(const std::string& message, Client* excludeClient = NULL);

	bool		isEmpty() const;
	bool		isOperator(int client_fd) const;
	bool		isClientInChannel(int client_fd) const;
	bool		isClientInChannel(std::string name) const;
	static bool	isAValidChannel(std::string name);
	bool		isRightPassword(std::string client_passw) const;
	bool		hasOperators();
	void		sendOperatorUpdate(Server& server);

	std::string				getName() const;
	std::string				getTopic() const;
	std::string				getKey() const;
	std::map<int, Client*>&	getClientMap();
	void					setKey(const std::string& password);

	//	JOIN:
	bool		isInvited(int client_fd) const;
	void		sendJoinChange(Client& client);
	void		sendClientInfo(Client& client, Server& server);

	// 	KICK:
	bool		canKickClient(int client_fd) const;
	void		kickClient(std::string clientOut, int kicker_fd, std::string reason);
	void		sendKickChange(std::string nameOut, int kicker_fd, std::string reason);

	// INVITE:
	bool		canInviteClient(int client_fd) const;
	void		inviteClient(Client* invitedClient, std::string nickname, int client_fd);

	//	TOPIC:
	bool		canChangeTopic(int client_fd) const;
	void		setTopic(Client client, std::string newTopic = "");
	void		sendTopicChange();

	//	MODE:
	bool		hasMode(char mode) const;
	bool		canChangeModes(int client_fd) const;
	void		setMode(const char mode, int client, std::string param = "");
	void		removeMode(const char mode, int client);
	void		sendModeChange(const char mode, char sign, int setter_fd, std::string param = "");

	//	PART:
	void		partClient(Client *client, Server *server, std::string reason = "");
	void		sendPartChange(int client_fd, std::string reason = "");

	//	HENDLER:
	static void		handleTopic(Client& client, Server& server, const std::vector<std::string>& params);
	static void		handleInvite(Client& client, Server& server, const std::vector<std::string>& params);
	static void		handleKick(Client& client, Server& server, const std::vector<std::string>& params);
	static void		handleJoin(Client& client, Server& server, const std::vector<std::string>& params);
	static void		handleMode(Client& client, Server& server, const std::vector<std::string>& params);
	static void		handlePart(Client& client, Server& server, const std::vector<std::string>& params);

private:
	std::string				name;
	std::string				topic;
	int						user_limit;
	std::string				topic_setter_info;
	time_t					topic_time;
	std::string				key;
	std::set<char>			modes;
	std::set<int>			operators;
	std::set<int>			invited;
	std::map<int, Client*>	clients;
};

#endif