#pragma once
#include <string>
#include "Session.hpp"
#include "Message.hpp"

struct Message;

namespace Reply
{
	extern const std::string endr;
	
	std::string PING_SERVER(Server *server, Session *session, Message message);

	std::string RPL_WELCOME_001(Server *server, Session *session, Message message);
	std::string RPL_YOURHOST_002(Server *server, Session *session, Message message);
	std::string RPL_CREATED_003(Server *server, Session *session, Message message);
	std::string RPL_MYINFO_004(Server *server, Session *session, Message message);
	std::string RPL_AWAY_301(Server *server, Session *session, Message message);

}