#include "Server.hpp"
#include "Client.hpp"
#include "InputParse.hpp"
#include <cstdlib>
#include <csignal>

volatile sig_atomic_t	g_shutdown = 0;

static void	signalHandler(int signum)
{
	(void)signum;
	g_shutdown = 1;
}

int		main(int argc, char *argv[]) {
	if (argc != 3)
	{
		std::cout << "Bad input, must be in this form: ./.. <port> <password> !" << std::endl;
		return 1;
	}
	
	struct sigaction	sa;
	std::memset(&sa, 0, sizeof(sa));

	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		std::cerr << "sigaction failed" << std::endl;
		return 1;
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
        std::cerr << "sigaction failed" << std::endl;
        return 1;
    }

	int port = std::atoi(argv[1]);
	std::string password = argv[2];
	InputParse::checkInput(port, password);
	
	Server	server(port, password);
	server.run();
}