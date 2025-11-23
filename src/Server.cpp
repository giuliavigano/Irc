#include "Server.hpp"
#include "Client.hpp"
#include <sys/socket.h>
#include <csignal>
#include <arpa/inet.h>

extern volatile sig_atomic_t	g_shutdown;

static std::string getHostname(int socketFd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(socketFd, (struct sockaddr*)&addr, &addr_len) == 0) {
        const char* ip = inet_ntoa(addr.sin_addr);
        if (ip)
            return std::string(ip);
    }
    return "unknown";
}

// VEDI SE LANCIARE INVECE UN'ECCEZIONE --> E GESTIRE L'ERRORE CON TRY CATCH (COSI L'OGETTO NON VIENE CREATO)
Server::Server(int _port, const std::string& password) : port(_port), passWord(password), serverName("ircserv") {
	if (!init()) {
		std::cerr << "Fatal: server initialization failed. Exiting!" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

// CHIUSURA SERVER E CLEANUP DI CLIENTS*
Server::~Server() {
	for(std::map<int, Client*>::iterator i = clients.begin(); i != clients.end(); ++i) {
		std::string message = "ERROR :Server is shutting down\r\n";
		send(i->first, message.c_str(), message.length(), 0);
	}
	for (std::map<std::string, Channel*>::iterator i = channels.begin(); i != channels.end(); ++i) {
		delete i->second;
	}
	channels.clear();
	for (std::map<int, Client*>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (i->first >= 0)
			close(i->first);
		delete i->second;
	}
	clients.clear();
	if (socketFd >= 0)
		close(socketFd);
}

// IN CASO DI FALLIMENTO DI QUESTE CHIAMATE NON POSSO GARANTIRE I/O NON BLOCCANTE --> ESCO
bool	Server::init() {
	socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0) {
		std::cerr << "socket() failed: " << strerror(errno) << ", while creating listener on port: " << port << std::endl;
		return false;
	}
	if (fcntl(socketFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "fcntl() failed: " << strerror(errno) << std::endl;
		close(socketFd);
		socketFd = -1;
		return false;
	}
	
	int		optionVal = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
	}

	struct	sockaddr_in	server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; // tipo di indirizzo
	server_addr.sin_port = htons(port); // htons() per convertire la porta scelta in ascolto in "network byte order" (perchè CPU e rete usano ordini di byte diversi)
	server_addr.sin_addr.s_addr = INADDR_ANY; // L'indirizzo IP su cui ascoltare (INADDR_ANY = Tutti gli indizzi ip della macchin)
	
	if (bind(socketFd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		std::cerr << "bind() failed: " << strerror(errno) << std::endl;
		close(socketFd);
		socketFd = -1;
		return false;
	}

	if (listen(socketFd, 128) < 0) {
		std::cerr << "listen() failed: " << strerror(errno) << std::endl;
		close(socketFd);
		socketFd = -1;
		return false;
	}

	std::ostringstream oss;
	oss << port;
	std::string str = oss.str();

	std::cout << "Server listening on " << getHostname(socketFd) + ":" + str << std::endl;
	return true;
}

// FD PIU ALTO FRA QUELLI SU CUI LAVORO (per select() -> monitora piu fd contemporaneamente per sapere quali sono pronti per fare operazioni di I/O input/output)
int		Server::get_maxFd() const {
	int		max = socketFd;
	for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
		if (it->first > max)
			max = it->first;
	}
	return max;
}

// controlla: accept() sarebbe bloccante (attende) --> quindi se arriva un segnale puo essere interrotta !
void	Server::handleNewConnection() {
	int	client_fd = accept(socketFd, NULL, NULL);
	if (client_fd < 0) {
		std::cerr << "accept() failed: " << strerror(errno) << std::endl; // perche voglio vedere tutti i log di errore altrimenti posso ridurli con SE if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) allora std::cer.... POI return;
		return;
	}
	else if (client_fd >= FD_SETSIZE) {
		close(client_fd);
		return;
	}
	else {
		fcntl(client_fd, F_SETFL, O_NONBLOCK);
		Client*	client = new Client(client_fd);
		clients[client_fd] = client;
	}
}

// GESTIRE ATTIVITÀ RICEVUTA DA CLIENT SOCKET --> DATI RICEVUTI (con recv(), estrai comandi e parsing ??)
void	Server::handleClient(int client_fd) {
	Client	*client = clients[client_fd];
	char	buffer[512]; // il protocollo irc specifica che i messaggi hanno max 512 byte (incluso \r\n)
	ssize_t	bytes = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytes < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
			return;
		std::cerr << "recv() failed on fd: " << client_fd << strerror(errno) << std::endl;
		close(client_fd);
		delete client;
		clients.erase(client_fd);
	} else if (bytes == 0) {
		std::cout << "Client " << client_fd << " disconnected" << std::endl;
		close(client_fd);
		delete client;
		clients.erase(client_fd);
	} else {
		buffer[bytes] = '\0';
		client->appendToBuffer(std::string(buffer, bytes));
		HandleBuffer::lookForCompleteMessage(*client, *this);
	}
}

// CICLO PRINCIPALE --> SEMPRE IN ASCOLTO PER CONNESSIONI/COMUNICAZIONI SERVER/CLIENT
void	Server::run() {
	while (!g_shutdown) {
		int		max_fd = get_maxFd();
		fd_set	read_set, write_set;
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_SET(socketFd, &read_set);
		for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
		{
			FD_SET(it->first, &read_set);

			if (it->second->hasDataToSend())
				FD_SET(it->first, &write_set);
		}
		int	result = select(max_fd + 1, &read_set, &write_set, NULL, NULL);
		if (result < 0) {
			if (errno == EINTR) // Interruzione da SEGNALE (Ctrl+C)
				continue;
			std::cerr << "select() failed: " << strerror(errno) << std::endl;
		}
		else if (result == 0)
			continue;
		else {
			if (FD_ISSET(socketFd, &read_set)) {
				handleNewConnection();
			}

			for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); )
			{
				if (FD_ISSET(it->first, &read_set)) {
					int	client_fd = it->first;
					++it;
					handleClient(client_fd);
				} else
					++it;
			}
			for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end();)
			{
				if (FD_ISSET(it->first, &write_set))
				{
					Client*	client = it->second;
					++it;
					handleClientWrite(client);
				} else
					++it;
			}
		}
	}
	std::cout << "Shutting down gracefully..." << std::endl;
	return ;
}

void	Server::handleClientWrite(Client* client)
{
	std::string&	write_buffer = client->getWriteBuffer();

	if (write_buffer.empty())
		return ;

	ssize_t	bytes_sent = send(client->get_fD(), write_buffer.c_str(), write_buffer.length(), MSG_NOSIGNAL);

	if (bytes_sent > 0)
		write_buffer.erase(0, bytes_sent);
	else if (bytes_sent < 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			std::cerr << "send() failed for client " << client->get_fD() << ": " << strerror(errno) << std::endl;
			int client_fd = client->get_fD();
			close(client_fd);
			delete client;
			clients.erase(client_fd);
		}
	}
	else if (bytes_sent == 0)
	{
		std::cout << "Client " << client->get_fD() << " disconnected." << std::endl;
		int	client_fd = client->get_fD();
		close(client_fd);
		delete client;
		clients.erase(client_fd);
	}
}

std::string	Server::getPsw() const
{
	return passWord;
}

std::map<int, Client*>	Server::getClientsMap() const
{
	return clients;
}

std::map<std::string, Channel*>&	Server::getChannelsMap() {
	return channels;
}

void	Server::sendReply(const std::string& code, const std::string& message, Client& client)
{
	std::string	reply = ":" + serverName + " " + code + " " + 
						(client.get_nickName().empty() ? "*" : client.get_nickName()) + 
						" " + message + "\r\n";
		
	client.appendToWriteBuffer(reply);
}

void	Server::ifRemoveChannel(const std::string& channelName) {
	std::map<std::string, Channel*>::iterator it = channels.find(channelName);

	if (it->second->isEmpty()) {
		delete it->second;
		channels.erase(it);
	}
}

const std::string&	Server::getServerName() const
{
	return serverName;
}

void	Server::eraseClient(Client*	client)
{
	int	client_fd = client->get_fD();
	close(client_fd);
	delete client;
	clients.erase(client_fd);
}