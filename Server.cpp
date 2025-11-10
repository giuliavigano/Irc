#include "Server.hpp"
#include <sys/socket.h>

// VEDI SE LANCIARE INVECE UN'ECCEZIONE --> E GESTIRE L'ERRORE CON TRY CATCH (COSI L'OGETTO NON VIENE CREATO)
Server::Server(int _port, const std::string& password) : port(_port), passWord(password) {
	if (!init()) {
		std::cerr << "Fatal: server initialization failed. Exiting!" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

// CHIUSURA SERVER E CLEANUP DI CLIENTS*
Server::~Server() {
	if (socketFd >= 0)
		close(socketFd);
	for (std::map<int, Client*>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (i->first >= 0)
			close(i->first);
		delete i->second;
	}
	clients.clear();
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
	// server listening on input port
	return true;
}

void	Server::run() {
	while (1) {
		
	}
}