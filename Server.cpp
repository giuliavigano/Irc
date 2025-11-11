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
	}
}

// CICLO PRINCIPALE --> SEMPRE IN ASCOLTO PER CONNESSIONI/COMUNICAZIONI SERVER/CLIENT
void	Server::run() {
	while (1) {
		int		max_fd = get_maxFd();
		fd_set	read_set;
		FD_ZERO(&read_set);
		FD_SET(socketFd, &read_set);
		for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
			FD_SET(it->first, &read_set);
		int	result = select(max_fd + 1, &read_set, NULL, NULL, NULL);
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
			for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ) {
				if (FD_ISSET(it->first, &read_set)) {
					int	client_fd = it->first;
					++it;
					handleClient(client_fd);
				} else
					++it;
			}
		}
	}
}

// RIPARTI DA HANDLE CLIENT E GESTIONE ERRORI SOPRATTUTTO ERRNO !!

//SE VUOI STAMPARE I LOG -->
//	struct sockaddr_in	client_addr;
//	socklen_t			addr_len = sizeof(client_addr);
//	int		client_fd = accept(socketFd, (struct sockaddr*)&client_addr, &addr_len);
//	char	*ip = inet_ntoa(client_addr.sin_addr);
//	int		port = ntohs(client_addr.sin_port);
//	std::cout<< "[" << client_fd << "] New connection from " << ip << ":" << port << std::endl
//	MI SERVONO QUESTE INFORMAZIONI SUL CLIENT ?

// FD_CLR 	per rimuovere un fd dal set --> MA RICREANDO IL SET OGNI VOLTA NON DOVREBBE SERVIRE !
// FD_ISSET	controlla se un fd è nel set (clients + socket server)


// GESTIONE ERRORI ERRNO:
// EWOULDBLOCK O EAGAIN --> nessuna connessione disponibile , socket non-bloccante --> NORMALE, RIPROVA DOPO!!
// EINTR --> INTERROTTO DA UN SEGNALE (Ctrl+C) --> RIPROVA ??

/*errno										Significato								Azione
EWOULDBLOCK / EAGAIN	Nessuna connessione disponibile (socket non-bloccante)	Normale, riprova dopo
EINTR					Interrotto da segnale									Riprova
EBADF					Socket invalido											Bug nel codice
EINVAL					Socket non in listen()									Bug nel codice
EMFILE					Troppi file aperti (limite processo)					Limite risorse
ENFILE					Troppi file aperti (limite sistema)						Limite risorse
ENOBUFS					Buffer insufficienti									Problema sistema*/