#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <list>
#include <string>

class Server;

class Client {
public:
	Client(int fd);
	~Client();
private:
	int			fD;
	std::string	nickName;
	std::string	userName;
	std::string	realName;
	bool					is_autenticated;
	std::list<std::string>	channels;
	std::string				buffer_text;

};

/*irssi traduce automaticamente i comandi dell'utente in comandi IRC e li invia al tuo server:

Il tuo server riceve questi comandi come stringhe attraverso recv() e deve:

Parsarli (estrarre il comando e i parametri)
Eseguire l'azione corrispondente
Inviare la risposta al client con send()*/

#endif