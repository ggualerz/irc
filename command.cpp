#include "command.hpp"
#include "debug.hpp"
#include "reply.hpp"
#include "error.hpp"
#include <vector>

static std::string removeCRLF(std::string const &str)
{
	std::string out = str;

	if (!out.empty() && (out[out.size() - 1] == '\n' || out[out.size() - 1] == '\r')) {
        out.erase(out.size() - 1);
    }
    if (!out.empty() && (out[out.size() - 1] == '\n' || out[out.size() - 1] == '\r')) {
        out.erase(out.size() - 1);
    }
	return(out);
}

static std::string commandToUpper(std::string const &str)
{
	std::string out = str;

	for (std::string::iterator it = out.begin(); it != out.end(); ++it) {
        *it = std::toupper(*it);
    }

	return(out);
}

static std::vector<std::string> parserPASS(std::string const &rawline)
{
	std::vector<std::string> result;
	std::string command;
	std::string arg;
	bool foundSpace = false;

	for (std::string::const_iterator it = rawline.begin(); it != rawline.end(); ++it)
	{
	    if (!foundSpace)
	    {
	        if (std::isspace(*it))
	            foundSpace = true;
	        else
	            command += *it; // Add to command until space is found
	    }
	    else
	        arg += *it; // Add to arg after space is found
	}


    
	command = commandToUpper(command);
    result.push_back(command);
	arg = removeCRLF(arg);
    result.push_back(arg);

    return result;
}

void	Command::pass(Server *server, Session *session, std::string rawline)
{
	std::vector<std::string> args = parserPASS(rawline);
	
	//Check if the correct command was called, yes I check a 2nd time
	if(args[0] != "PASS" || args[1].empty() == true)
	{
		Debug::Warning("PASS should not be called, arg0: " + args[0] + " arg1 " + args[1] + "\n rawline: \n" + rawline);
		return;
	}

	if(session->getPassIsSet() == true)
	{
		Error::ERR_ALREADYREGISTRED_462(server, session);
		// server->killSession(session->getFdSocket()); //Disconect the user ?
		return;
	}
	

	//Passwd check
	if(server->checkPassword(args[1]) == false)
	{
		Error::ERR_PASSWDMISMATCH_464(server, session);
		return;
	}
	else
	{
		Reply::RPL_WELCOME_001(server, session);
		session->setPassTrue();
		return;
	}
	return;
}


// send(i, msg.c_str(), msg.length(), 0);