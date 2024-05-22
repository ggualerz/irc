
#pragma once

#include <iostream>
#include <string>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "Channel.hpp"
#include "Session.hpp"

#include "command.hpp"
#include "debug.hpp"

#define BUFFER_SIZE  512
#define BACKLOG  10

class Session;

class Server
{
private:
	#include "ServerExceptions.hpp"

	int	_fd_socket; // (Server) a socket is basically a file descriptor++
	struct sockaddr_in _address_socket; // (Server) struct who contain socket info

	// Not used outside of server class no need for getters
	fd_set _sessions_fd;
	//
	std::string _hostname; // hostname of the irc server
	std::string _password; // password to join the server, no getter for obvious reasons
	uint16_t	_port; // uint16_t for unsigned int of 16 bit  0 to 65535 because a port can only be in this range according to the OS
	bool _should_i_end_this_suffering; //Bool set to true to kill the server
	// uint const _buffer_size = 512; // Size of the buffer of the socket, basically will truncate msg over X bytes
	// uint const _max_connections_queue = 10; // Maximum cnx pending into the socket queue before getting accepted, also called BACKLOG

	// std::map<std::string, Channel> _channels; // map of the channels, channel name is the key of the map, vallue is the object channel
	std::map<int, Session *> _sessions; //map of the sessions, session fd is the key
	typedef std::string (*CommandPtr)(Server *, Session *, std::string);
	std::map<std::string, CommandPtr > _commands;
	
	//init method, should only be called by constructor
	void createSocket(void);
	void bindSocket(void);
	void launchListen(void);
	void initSessionsFds(void);
	void mapCommands(void);
	void handleConnections(void);

	
public:
	Server(std::string hostname, std::string pwd, uint16_t port);
	~Server();

	//Getters
	std::string const &getHostName(void) const
		{return (this->_hostname);}
	bool checkPassword(std::string passwordToCheck) const
		{return (passwordToCheck == this->_password);}
	uint16_t const &getPort(void) const
		{return (this->_port);}
	int	const &getFdSocket(void) const
		{return (this->_fd_socket);}
	sockaddr_in const &getAddressSocket(void) const
		{return (this->_address_socket);}
	socklen_t getLenSocket(void) const
		{return(sizeof(this->_address_socket));}
	bool getKill(void) const
		{return (this->_should_i_end_this_suffering);}
	int  getBufferSize(void) const
		{return (BUFFER_SIZE);} // MAKE SOMETHING CLEAN JPTA
	int  getBackLog(void) const
		{return (BACKLOG);}  // MAKE SOMETHING CLEAN JPTA

	//Methods
	void killSession(int const session_fd);
	
};