#include "Server.hpp"
#include <unistd.h>
#include <csignal>
volatile sig_atomic_t ctrlc_pressed = 0;

int main(int argc, char **argv)
{
	//Do parsing shit here
	//check if port is valid
	if (argc != 3)
	{
		Debug::Error("Invalid use it should be \"./ircserv <port> <password>\"");
		std::exit(1);
	}
	char hostname[256];
	int port = atoi(argv[1]);
	std::string pwd(argv[2]);
	
	if (gethostname(hostname, sizeof(hostname)) != 0)
	{
		Debug::Error("Invalid hostname JPTA");
		std::exit(1);
	}

	Server irc(std::string(hostname), pwd , port);
	
}