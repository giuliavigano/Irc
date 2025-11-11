#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <list>
#include <string>

class Server;

class Client {
public:
	Client(int fd);
	~Client();

	int			get_fD() const;
	std::string	get_nickName() const;
	std::string	get_userName() const;

	void		setNickName(const std::string& nickname);
	void		setUserName(const std::string& userName);

	bool		isRegistered() const;
	bool		isAutenticated() const;
	
private:
	int						fD;
	std::string				nickName;
	std::string				userName;
	bool					is_autenticated;
	std::list<std::string>	channels;
	std::string				buffer;

};

/*irssi traduce automaticamente i comandi dell'utente in comandi IRC e li invia al tuo server:

Il tuo server riceve questi comandi come stringhe attraverso recv() e deve:

Parsarli (estrarre il comando e i parametri)
Eseguire l'azione corrispondente
Inviare la risposta al client con send()*/

#endif