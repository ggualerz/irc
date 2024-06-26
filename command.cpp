#include "command.hpp"
#include "debug.hpp"
#include "reply.hpp"
#include "error.hpp"
#include "Message.hpp"
#include <vector>




std::string Command::cap(Server *server, Session *session, Message message)
{
	(void)server;
	(void)session;
	(void)message;
	return("");
}

std::string	Command::pass(Server *server, Session *session, Message message)
{
	
	// //Check if the correct command was called, yes I check a 2nd time
	// if(args[0] != "PASS" || args[1].empty() == true)
	// {
	// 	Debug::Warning("PASS should not be called, arg0: " + args[0] + " arg1 " + args[1] + "\n rawline: \n" + rawline);
	// 	return ("");
	// }
	if(message.params.empty())
	{
		return(Error::ERR_NEEDMOREPARAMS_461(server, session, message));
	}
	if(session->getPassIsSet() == true)
	{
		return(Error::ERR_ALREADYREGISTRED_462(server, session));
		// server->killSession(session->getFdSocket()); //Disconect the user ?
	}
	
	
	//Passwd check
	if(server->checkPassword(message.params[0]) == false)
	{
		return(Error::ERR_PASSWDMISMATCH_464(server, session));
	}
	else
	{
		session->setPassTrue();
		return("");
		// return(Reply::RPL_WELCOME_001(server, session)); // FOR TEST
	}
}



std::string	Command::nick(Server *server, Session *session, Message message)
{
	
	if(session->getPassIsSet() == false)
		return("");
	if(session->getAuthenticated() == true)
		return(Error::ERR_ALREADYREGISTRED_462(server,session));
	if(message.params.empty())
		return(Error::ERR_NEEDMOREPARAMS_461(server, session, message));
	if(message.params[0].length() < 1 || message.params[0].length() > 9)
		return(Error::ERR_ERRONEUSNICKNAME_432(server, session, message));
	for(size_t i = 0; i < message.params[0].size(); i++)
	{
		if(!(Utils::isAllowedNickCharacter(message.params[0][i])))
			return(Error::ERR_ERRONEUSNICKNAME_432(server, session, message));
	}
	if(server->getSession(message.params[0]) != NULL)
		return(Error::ERR_NICKNAMEINUSE_433(server, session, message));
	session->setNickName(message.params[0]);
	if(session->authenticate())
	{
		return(	Reply::RPL_WELCOME_001(server,session,message) +\
				Reply::RPL_YOURHOST_002(server,session,message) +\
				Reply::RPL_CREATED_003(server,session,message) +\
				Reply::RPL_MYINFO_004(server,session,message) +\
				Reply::PING_SERVER(server,session,message));
	}
	else
		return("");
}

std::string	Command::user(Server *server, Session *session, Message message)
{
	if(session->getPassIsSet() == false)
		return("");
	if(session->getAuthenticated() == true)
		return(Error::ERR_ALREADYREGISTRED_462(server,session));
	if(message.params.empty() || (message.params.size() < 3 && (!(message.payload.empty()))) || (message.params.size() < 4 && (message.payload.empty())) )
		return(Error::ERR_NEEDMOREPARAMS_461(server, session, message));
	session->setUserName(message.params[0]);
	if(message.params.size() == 4)
		session->setRealName(message.params[3]);
	else
		session->setRealName(message.payload);
	if(session->authenticate())
	{
		return(	Reply::RPL_WELCOME_001(server,session,message) +\
				Reply::RPL_YOURHOST_002(server,session,message) +\
				Reply::RPL_CREATED_003(server,session,message) +\
				Reply::RPL_MYINFO_004(server,session,message) +\
				Reply::PING_SERVER(server,session,message));
	}
	else
		return("");
}


std::string	Command::quit(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	std::string msg;
	msg = "ERROR :Respond to QUIT from client";
	session->addSendBuffer(msg);
	msg = Utils::getUserPrefix(server, session) + "QUIT :" + "Quit: " + message.payload + Reply::endr;
	Channel *tmp_chan = session->getChannel();
	if(tmp_chan)
	{
		std::string chan_name = tmp_chan->get_name();
		if (tmp_chan->get_users().size() > 1)
		{
			Utils::sendToChannel(server, tmp_chan, session->getNickName(), msg, chan_name);
			tmp_chan->rm_user(session->getNickName());
		}
		else
			server->removeChannel(chan_name);
	}
		
	// {
		// std::string msg = ":" +session->getNickName() + " !d@" + server->getHostName() + "QUIT :" + "Quit: " + message.payload + Reply::endr; // A CHECK
		// std::vector<std::string> lst_user = tmp_chan->get_users();
		// for(size_t i = 0; i < lst_user.size(); i++)
		// {
		// 	server->getSession(lst_user[i])->addSendBuffer(msg);
		// }
	// }
	server->killSession(session->getFdSocket(), true);
	return("");
}

// std::string	Command::notice(Server *server, Session *session, Message  message)
// {
// 	if(session->getAuthenticated() == false)
// 		return ("");
// 	// If message.params[0] == & OU #, alors on boucle sur tous les utilisateurs de ce channel pour send le message
// 	if (!server->getSession(message.params[0]))
// 		return ("");
// 	std::string	msg = Utils::getUserPrefix(server, session) + "PRIVMSG " + message.params[0] + " :" + message.payload + Reply::endr;
// 	//send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
// 	server->getSession(message.params[0])->addSendBuffer(msg);
// 	return ("");
// }

std::string	Command::privmsg(Server *server, Session *session, Message  message)
{
	std::string msg;
	std::string command_name;
	if(message.command == "PRIVMSG")
		command_name = "PRIVMSG";
	else
		command_name = "NOTICE";
	if(session->getAuthenticated() == false)
		return ("");
	if (message.params.size() > 2)
		return (Error::ERR_TOOMANYTARGETS_407(server, session, message));
	if (message.params.size() == 0)
		return (Error::ERR_NORECIPIENT_411(server, session, message));
	if (message.payload.empty() && message.params.size() == 1)
		return (Error::ERR_NOTEXTTOSNED_412(server, session, message));
	std::vector<std::string> targets = Utils::split(message.params[0], ',');
	for(size_t i = 0; i < targets.size(); i++)
	{
		if(targets[i].find('#') != std::string::npos || targets[i].find('&') != std::string::npos) //If PRIVMSG TO A CHANNEL
		{
			if(!Utils::isValidChannelName(targets[i]))
				return (Error::ERR_BADCHANMASK_476(server, session, targets[i]));
			Channel *target = server->getChannel(targets[i].substr(targets[i].find('#'))); //remove possible flags
			if(target == NULL)
				return(Error::ERR_NOSUCHCHANNEL_403(server, session, message));
			if(session->getChannel() != target)
				return(Error::ERR_CANNOTSENDTICHAN_404(server, session, message));
			if(!message.payload.empty())
				msg = Utils::getUserPrefix(server, session) + command_name + " " + targets[i] + " :" + message.payload + Reply::endr;
			if(!msg.empty() && targets.size() == 1)
				Utils::sendToChannel(server, target, session->getNickName(),  msg, targets[i]);
			else if(!msg.empty())
				Utils::sendToChannel(server, target, "sender dummy to send the message evem to the sender",  msg, targets[i]);
		}
		else //If PRIVMSG TO AN USER
		{
			Message tmp = message;
			tmp.params[0] = targets[i];
			if(!server->getSession(targets[i]))
				return (Error::ERR_NOSUCHNICK_401(server, session, tmp));
			if(session->getAwayStatus() != "" && command_name != "NOTICE")
				return (Reply::RPL_AWAY_301(server, session, message));
			if(!message.payload.empty())
				msg = Utils::getUserPrefix(server, session) + command_name + " " + targets[i] + " :" + message.payload + Reply::endr;
			else if(!message.params[1].empty())
				msg = Utils::getUserPrefix(server, session) + command_name + " " + targets[i] + " :" + message.params[1] + Reply::endr; //Add for more natural command line

			server->getSession(targets[i])->addSendBuffer(msg);
		}
	}
	
	return ("");
}

//Only from the client to the server
std::string	Command::ping(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return("");
	if(message.payload.empty() && message.params.size() == 0)
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	//Reply Pong
	if(!message.payload.empty())
		return(":" + server->getHostName() + " PONG :" + message.payload + Reply::endr);
	if(message.params.size() != 0)
		return(":" + server->getHostName() + " PONG :" + message.params[0] + Reply::endr);
	else
		return("");
}
//Only from the client to the server
std::string	Command::pong(Server *server, Session *session, Message  message)
{
	(void)server;
	if(session->getAuthenticated() == false)
	{
		Debug::Warning("Received an unexpected PONG while session not auth");
		return("");
	}
	if(session->getWaitPong() == true && message.payload == session->getNickName())
	{
		Debug::Info("Received a correct pong while waiting it");
		session->newPong();
		return("");
	}
	else
	{
		Debug::Warning("Received an unexpected PONG while not waiting it or the token is not valid");
		return("");
	}}

//CHANNELS-----------------------------------------------------

std::string	Command::part(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if (message.params.size() < 1)
		return (Error::ERR_NEEDMOREPARAMS_461(server, session, message));
	std::string reason;
	if (message.payload.empty())
		reason = "Unknown";
	else
		reason = message.payload;
	
	Channel *channel = server->getChannel(message.params[0]);
	if(channel == NULL)
		return(Error::ERR_NOSUCHCHANNEL_403(server, session, message));
	if(session->getChannel() != channel)
		return(Error::ERR_NOTONCHANNEL_442(server,session,message));
	if(channel->get_users().size() == 1)
	{
		session->setChannel(NULL);
		std::string msg = Utils::getUserPrefix(server,session) + "PART " + channel->get_name() + " " + reason + Reply::endr;
		server->removeChannel(channel->get_name());
		return(msg);
	}
	else
	{
		channel->rm_user(session->getNickName());
		session->setChannel(NULL);
		
		std::string msg = Utils::getUserPrefix(server,session) + "PART " + channel->get_name() + " " + reason + Reply::endr;
		Utils::sendToChannel(server,channel,session->getNickName(), msg, message.params[0]);
		return(msg);
	}
	return("");
}

std::string	Command::join(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() == 1 && message.params[0] == "0") // see RFC Join cmd
		return(Command::part(server, session, message));
	if(session->getChannel() != NULL)
		return(Error::ERR_TOOMANYCHANNELS_405(server,session,message));
	if(message.params.size() < 1)
		return(Error::ERR_NEEDMOREPARAMS_461(server, session, message));
	if(!Utils::isValidChannelName(message.params[0]))
		return(Error::ERR_BADCHANMASK_476(server,session, message.params[0]));

	if(message.params.size() == 1)
		message.params.push_back(""); //add a blank password to channel
	if(message.params.size() > 2) //if try to join more than a server
		return(Error::ERR_TOOMANYCHANNELS_405(server,session,message));
	Channel *target_chan;
	target_chan = server->getChannel(message.params[0]);
	bool newchan = false;
	if(target_chan == NULL)
	{
		newchan = true;
		Debug::Info("Channel name not found, lets create a new one");
		target_chan = new Channel(message.params[0], message.sender);
		target_chan->add_op(session->getNickName());
		target_chan->set_founder(session->getNickName());
		server->addChannel(message.params[0], target_chan);
	}
	std::string chan_name = target_chan->get_name();
	if(!target_chan->get_pw().empty() && !target_chan->checkPw(message.params[1])) //if chan have a password and the password don't match
		return(Error::ERR_BADCHANNELKEY_475(server, session, message.params[0]));
	if(target_chan->get_inviteonly() && target_chan->getUserInvited(session->getNickName()) == false)
		return(Error::ERR_INVITEONLYCHAN_473(server, session, chan_name));
	if(!target_chan->getMaxUsers() == 0 && target_chan->get_nmemb() >= target_chan->getMaxUsers())
		return(Error::ERR_CHANNELISFULL_471(server,session,chan_name));
	target_chan->add_user(session->getNickName()); //Oui j'ai vu noé que tu renvoie un int, le probleme c'est que ça gere pas si le channel est en mode invite ou non
	std::string join_msg = Utils::getUserPrefix(server, session) +  "JOIN " + chan_name + Reply::endr;
	if(newchan == true)
		join_msg += ":" + server->getServerName() + " MODE " + chan_name + " +o " + session->getNickName() + Reply::endr;//inform that the user who create the chan is op
	Utils::sendToChannel(server, target_chan, session->getNickName(), join_msg,chan_name); //send join message of the user to all other users of this chan
	session->setChannel(target_chan);
	std::string msg = join_msg;
	if(target_chan->get_topic() != "")
	{
		msg += Reply::RPL_TOPIC_332(server, session, target_chan) + Reply::RPL_TOPICWHOTIME_333(server, session, target_chan);
	}
	msg += Reply::RPL_NAMREPLY_353(server, session, target_chan);
	msg += Reply::RPL_ENDOFNAMES_366(server, session, target_chan->get_name());
	return(msg);

}

std::string	Command::who(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.empty())
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	std::string msg;
	if(message.params[0] == "0" && message.params.size() == 1)
	{
		msg = Reply::RPL_WHOREPLY_352(server, session,NULL, NULL) + Reply::RPL_ENDOFWHO_315(server, session, message);
		return(msg);
	}
	else if(message.params[0][0] == '#' || message.params[0][0] == '&')//If target is a channel
	{
		Channel *target_chan = server->getChannel(message.params[0]);
		if(target_chan == NULL)
			return(Error::ERR_NOSUCHCHANNEL_403(server, session, message));
		msg = Reply::RPL_WHOREPLY_352(server, session,target_chan, NULL) + Reply::RPL_ENDOFWHO_315(server, session, message);
	}
	else
	{
		Session *target_session = server->getSession(message.params[0]);
		if(target_session == NULL)
			return(Error::ERR_NOSUCHNICK_401(server, session, message));
		msg = Reply::RPL_WHOREPLY_352(server, session,NULL, target_session) + Reply::RPL_ENDOFWHO_315(server, session, message);
	}
	return(msg);
}

std::string	Command::topic(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() == 0)
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	Channel *chan = server->getChannel(message.params[0]);
	if(chan == NULL)
		return(Error::ERR_NOSUCHCHANNEL_403(server,session,message));
	if(session->getChannel() != chan)
		return(Error::ERR_NOTONCHANNEL_442(server,session,message));
	//reply chan topic 
	if(message.params.size() == 1 && message.payload.empty())
	{
		if(chan->get_topic() == "")
			return(Reply::RPL_NOTOPIC_331(server,session,chan));
		return(Reply::RPL_TOPIC_332(server,session,chan) + Reply::RPL_TOPICWHOTIME_333(server,session,chan));
	}
	if(message.params.size() == 1 && !message.payload.empty())
	{
		if(chan->get_topicrestricted() == true && chan->is_op(session->getNickName()) == false)
			return(Error::ERR_CHANOPRIVSNEEDED_482(server,session,message.params[0]));
		std::string topic;
		if(message.payload == ":")
			topic = "";
		else
			topic = message.payload;
		chan->set_topic(topic, session->getNickName());

		std::string msg = Utils::getUserPrefix(server,session) + "TOPIC " + chan->get_name() + " " + message.payload + Reply::endr;
		Utils::sendToChannel(server,chan,session->getNickName(),msg,message.params[0]);
		return(msg);
	}
	return("");
}

std::string	Command::names(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	std::string msg;
	if(message.params.empty() || message.params[0] == server->getHostName() || message.params[0] == "ft_irc")//If NAMES 0, list all chans, also if name is hostname or if the name is specifically ft_irc
	{
		std::map<std::string, Channel*> chans = server->getChannels();
		std::map<std::string, Channel*>::iterator it1;
		if (chans.size() > 0)
		{
		    it1 = chans.begin();
		    while (it1 != chans.end())
		    {
				msg+= Reply::RPL_NAMREPLY_353(server,session,it1->second);
				msg += Reply::RPL_ENDOFNAMES_366(server, session, it1->second->get_name());
		        ++it1;
		    }
		}
		std::map<int, Session*> sess = server->getSessions();
		std::map<int, Session*>::iterator it2;
		if (sess.size() > 0)
		{
			std::string msg_nochannel = Utils::getServerPrefix(server, session, "353") + "= * :"  ;
			size_t user_nochannel = 0;
		    it2 = sess.begin();
		    while (it2 != sess.end())
		    {
				if(it2->second->getChannel() == NULL)
				{
					++user_nochannel;
					msg_nochannel += it2->second->getNickName() + " ";
				}
		        ++it2;
		    }
			msg_nochannel += Reply::endr;
			msg_nochannel += Utils::getServerPrefix(server,session,"366") + "* :End of /NAMES list" + Reply::endr;
			if(user_nochannel > 0)
				msg += msg_nochannel;
		}

	}
	else
	{
		for(size_t i = 0; i < message.params.size(); i++)
		{
			if(server->getChannel(message.params[i]) == NULL)
				msg += Reply::RPL_ENDOFNAMES_366(server, session, message.params[i]);
			else
			{
				msg+= Reply::RPL_NAMREPLY_353(server,session,server->getChannel(message.params[i]));
				msg += Reply::RPL_ENDOFNAMES_366(server, session,  message.params[i]);
			}
		}	
	}

	return(msg);
}

std::string	Command::list(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	std::map<std::string,Channel *> channels = server->getChannels();
	std::map<std::string,Channel *>::iterator it = channels.begin();
	std::string msg;
	size_t channel_cout = 0;
	if(message.params.empty())
	{
		while(it != channels.end())
		{
			if(it->second != NULL)
			{
				channel_cout++;
				msg += Reply::RPL_LIST_322(server,session, it->second);
			}
			it++;
		}
		if(channel_cout > 0)
			msg = Reply::RPL_LISTSTART_321(server,session) + msg + Reply::RPL_LISTEND_323(server,session);
		else
			msg = Reply::RPL_LISTEND_323(server,session);

	}
	else
	{
		std::vector<std::string> chan_lst = Utils::split(message.params[0], ',');
		for(size_t i = 0; i < chan_lst.size(); i++)
		{
			if(server->getChannel(chan_lst[i]) == NULL)
				msg += Error::ERR_NOSUCHCHANNEL_403(server,session,message);
			else
				msg += Reply::RPL_LIST_322(server,session,server->getChannel(chan_lst[i]));
		}
		msg = Reply::RPL_LISTSTART_321(server,session) + msg + Reply::RPL_LISTEND_323(server,session);
	}
	return(msg);
}

std::string	Command::invite(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() < 2)
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	
	if(server->getChannel(message.params[1]) == NULL)
	{
		Message tmp;
		tmp = message;
		tmp.params[0] = message.params[1];
		return(Error::ERR_NOSUCHCHANNEL_403(server,session,tmp));
	}
	Channel *chan = server->getChannel(message.params[1]);
	if(chan->get_inviteonly() == true && chan->is_op(session->getNickName()) == false)
		return(Error::ERR_CHANOPRIVSNEEDED_482(server,session,message.params[1]));
	if(server->getSession(message.params[0]) == NULL)
		return(Error::ERR_NOSUCHNICK_401(server,session,message));
	if(session->getChannel() != chan)
	{
		Message tmp;
		tmp = message;
		tmp.params[0] = message.params[1];
		return(Error::ERR_NOTONCHANNEL_442(server,session,tmp));
	}
	if(chan->is_user(message.params[0]) == true)
		return(Error::ERR_USERONCHANNEL_443(server,session,message.params[0],message.params[1]));
	if(chan->get_inviteonly() == true && chan->is_op(session->getNickName()) == true)
	{
		chan->invite_user(message.params[0]);
	}
	std::string msg = Reply::RPL_INVITING_341(server,session,message.params[0],message.params[1]);
	server->getSession(message.params[0])->addSendBuffer(msg);
	return(msg);
}

std::string	Command::kick(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() < 2)
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	if(server->getChannel(message.params[0]) == NULL)
		return(Error::ERR_NOSUCHCHANNEL_403(server,session,message));
	Channel *chan = server->getChannel(message.params[0]);
	if(chan->is_op(session->getNickName()) == false)
		return(Error::ERR_CHANOPRIVSNEEDED_482(server,session,message.params[0]));
	if(session->getChannel() != chan)
		return(Error::ERR_NOTONCHANNEL_442(server,session,message));
	if(chan->is_user(message.params[1]) == false)
		return(Error::ERR_USERNOTINCHANNEL_441(server,session,message.params[1],message.params[1]));
	std::string reason;
	if(message.payload.empty())
		reason = " Reason not known";
	else
		reason = " " + message.payload;
	
	
	std::string msg = Utils::getUserPrefix(server,session) + "KICK " + message.params[0] + " " + message.params[1] + reason + Reply::endr;
	Utils::sendToChannel(server,chan,session->getNickName(),msg,message.params[0]);
	chan->rm_user(message.params[1]);
	server->getSession(message.params[1])->setChannel(NULL);
	return(msg);

}

std::string	Command::motd(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() != 0)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	std::string msg;
	msg = Reply::RPL_MOTDSTART_375(server,session,server->getHostName());
	msg += Reply::RPL_MOTD_372(server,session,"Welcome out server");
	msg += Reply::RPL_MOTD_372(server,session,"We are proud to show you this masterpiece done in two weeks");
	msg += Reply::RPL_MOTD_372(server,session,"Use /help to guide you throught the server");
	msg += Reply::RPL_MOTD_372(server,session,"Some easter eggs are well hiden, find them all !");
	msg += Reply::RPL_MOTD_372(server,session,"Thanks to thepaqui && bbourret for the help provided during the project");
	msg += Reply::RPL_MOTD_372(server,session,"Thanks to fbardeau too");
	msg += Reply::RPL_MOTD_372(server,session,"This project was a pure pleasure to make");
	msg += Reply::RPL_MOTD_372(server,session,"02 June 2024, ggualerz");
	msg += Reply::RPL_ENDOFMOTD_376(server,session);
	return(msg);
}

std::string	Command::version(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() != 0)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	return(Reply::RPL_VERSION_351(server,session,server->getHostName()));
}

std::string	Command::admin(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() != 0)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	std::string msg = Reply::RPL_ADMINME_256(server,session,server);
	msg += Reply::RPL_ADMINLOC1_257(server,session,server);
	msg += Reply::RPL_ADMINLOC2_258(server,session,server);
	msg += Reply::RPL_ADMINEMAIL_259(server,session,server);
	return(msg);
	
}

std::string	Command::lusers(Server *server, Session *session, Message  message)
{
	(void)message;
	if(session->getAuthenticated() == false)
		return ("");
	std::string msg;
	msg = Reply::RPL_LUSERCLIENT_251(server,session);
	msg += Reply::RPL_LUSEROP_252(server,session);
	msg += Reply::RPL_LUSERUNKNOWN_253(server,session);
	msg += Reply::RPL_LUSERCHANNELS_254(server,session);
	msg += Reply::RPL_LUSERME_254(server,session);
	msg += Reply::RPL_LOCALUSERS_265(server,session);
	return(msg);
}

std::string	Command::time(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() != 0)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	return(Reply::RPL_TIME_391(server,session,server));
}

std::string	Command::stats(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.size() < 1)
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	if(message.params.size() > 1)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	if(message.params[0][0] != 'u')
		return(Error::ERR_NOPRIVILEGES_481(server,session,message));
	if(message.params[0][0] == 'u')
		return(Reply::RPL_STATSUPTIME_242(server,session));
	return("");
}

std::string	Command::help(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	std::string msg;
	// /HELP or /HELP help, will show that
	if(message.params.size() == 0 || Utils::strToUpper(message.params[0]) == "HELP")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,"*","Summary of commands");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Try to use \"/help <command>\" for more information");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","if on HexChat replace \"/help\" by \"/srvhelp\"");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Connection commands:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","PASS, NICK, USER, PING, PONG, QUIT");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Channel operations:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","JOIN, PART, TOPIC, NAMES, LIST, INVITE, KICK");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Queries and commands:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","MOTD, VERSION, ADMIN, LUSERS, TIME, STATS, HELP, INFO, MODE");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Sending Messages:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","PRIVMSG, NOTICE, WALLOPS");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","User-Based Queries:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","WHO, WHOIS, AWAY");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","Some commands are not implanted in this server");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","For various reason, we didn't code them.");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","List of unsuported commands:");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","CAP, AUTHENTICATE, OPER, ERROR, CONNECT, WHOWAS, KILL, REHASH");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","RESTART, SQUIT, LINKS, USERHOST");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_HELPTXT_705(server,session,"*","");
		msg += Reply::RPL_ENDOFHELP_706(server,session,"*","End of /help");
		return(msg);
	}

	std::map<std::string, CommandPtr> commands = server->getCommands();
	std::string command_lower = Utils::strToLower(message.params[0]);
	std::string command_uper = Utils::strToUpper(message.params[0]);
	//Help command that does't exist
	if(message.params.size() != 0 && commands[command_uper] == NULL) //Command not found
		return(Error::ERR_HELPNOTFOUND_524(server,session,command_lower));
	//Help of a command that is not implemented, except CAP
	if(commands[command_uper] == &(Command::notimplanted)) //Command not implanted
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"Command not implanted");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command is part of the RFC 2812");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"According to the subject of ft_irc, a 42school project");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"We choosed to not add this command, it could be because");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"its' related to SSL, SASL, MultiServer, Server Operator...");
		msg += Reply::RPL_ENDOFHELP_706(server,session,command_lower,"End of /help");
		return(msg);
	}
	else if(command_uper == "CAP")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"CAP command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The CAP command is simply ignored");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command should not be used by an user, only the client");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The CAP command is use to negociate capabilities between client and server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "PASS")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"PASS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The PASS command is waited before any other commands");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"It will be used for the client to give the password");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"If the password is correct, the server wait for");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"NICK & USER command to complete the authentification");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command should not be used by an user, only the client");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The command can be skipped in some scenario, then wait other commands");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "NICK")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"NICK command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The NICK command can be done after a sucessfull PASS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"If this command succeed and a previous USER command succeed too");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The user will be authenticated on the server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"It's used for set the nickname of the user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The nickname cannot be duplicated and is case insensitive");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The nickname should be at least 1 char and maximum 9 char long");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The nickname should contain only those char:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"a-z,A-Z,0-9,[,],{,},-,_");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command should not be used by an user, only the client");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command cannot be send again if it succeed");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Same as previous, but the nickname can be redifined");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The nick should be unique server wide, not network wide");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "USER")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"USER command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The USER command can be done after a sucessfull PASS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"If this command succeed and a previous NICK command succeed too");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The user will be authenticated on the server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command will set the username and the realname of the user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to the rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "PING")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"PING command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The PING command is issued to check the lag between the client/server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command is emited by the server after a sucessfull auth");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command should contain a parameter, this parameter will be return by PONG");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to the rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "PONG")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"PONG command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The PONG command is replied from a PING to check the lag between the client/server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This command should contain a parameter");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"This parameter will be the param of the previous PING");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to the rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "QUIT")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"QUIT command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The QUIT command will terminate the user connection");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"and inform all other member of the channel with the reason as parameter");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"the reason is optional");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/quit [reason]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to the rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "JOIN")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"JOIN command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The JOIN command will add you to a channel if your meet the conditions");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"only one channel joined per user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"channel starts with # or &");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if the channel doesnt exist, create it then op yourself");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/join #channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"JOIN can take multiple channels, #channel are network wide, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "PART")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"PART command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The PART command remove you from the channel and inform the other members");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"only one channel as argument");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if you are the last member, destroy the channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/part [#channel]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"PART can take multiple channels, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "TOPIC")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"TOPIC command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The TOPIC show, set or clear the channel topic");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"without argument it show it and it show who set it and when");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"with an argument after :, it set it");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"with only ':' as argument it clear it");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"with MODE command +t, only ops can perform this action");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/topic [:topic text]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "NAMES")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"NAMES command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The NAMES show users of specific channels or all channels");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"it can take as argument a channel, multiple channels to show channel member");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"with no argument (depend client), hostname argument or \"ft_irc\" string");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"it shows all users of the server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/names [channel,channel,..]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same except for the all users show, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "LIST")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"LIST command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The LIST show information about all channels or a list of them");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"without an argument, show all chan, the number of user and the topic");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"with a list of channels, the same as before but on those channels");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/list [channel,channel,..]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "INVITE")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"INVITE command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The INVITE command invite an user in the channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The user who invite should be on the channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if the channel is in MODE command +i, only ops can invite users");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"it will ad the user to invited list");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if the MODE command -i is set, all users can invite");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"but the invite list will not change");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"in the MODE -i, all user can join with or without an invitation");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if the mode is set back to +i, all previous invitation would be canceled");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/invite nickname channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "KICK")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"KICK command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The KICK command kick from channel an user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"It need op privilege");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"It can kick with a comment");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/kick nickname channel [comment]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same but can kick multiple user in the same time, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "MOTD")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"MOTD command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The MOTD print the Message of the day");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/motd");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "VERSION")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"VERSION command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The VERSION print the server version and version comment");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The VERSIO alias is set to bypass weird handle of it by HexChat");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/version (/versio)");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Can print the version of a remote server on the network, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "ADMIN")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"ADMIN command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The ADMIN print the administrative info about the hoster of the server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/admin");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Can print the admin of a remote server on the network, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "LUSERS")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"LUSERS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The LUSERS print stats about local users");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/lusers");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Print stats about global user too, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "TIME")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"TIME command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The TIME print local server time");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/time");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Print time about remote server on network too, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "STATS")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"STATS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The STATS print different stats according to a flag");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"flag(l): stats about each connections");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"flag(u): server uptime");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/stats flag");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"handle more stats, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "INFO")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"INFO command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The INFO print server info");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/info");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "MODE")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"MODE command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The MODE add or remove a channel mode");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Only a channel operator can use this command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"(+i) set the channel on invite from op only, (-i) remove the restriction");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"(+t) set the channel on topic set by op only, (-t) remove the restriction");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"(+k) set the channel password, (-k) remove the restriction");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"(+o) Op the user, (-o) unOp the user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"(+l) set an user limite on the channel, (-l) remove the restriction");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/mode +/-flag [argument]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same with multiple flag per exec and more flags, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "PRIVMSG")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"PRIVMSG command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The PRIVMSG send a message to an user or channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"this command take multiple target separated by ','");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/privmsg user[,channel,user,..] :my message");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "NOTICE")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"NOTICE command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The NOTICE send a message to an user or channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"No automatic message would be replied, some clients show it weirdly");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"this command take multiple target separated by ','");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/notice user[,channel,user,..] :my message");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Pretty much the same, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "WHO")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"WHO command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The WHO print information about all user in a specified channel");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"if the channel is specified to '0' it will show all user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/who #channel or 0");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"a bit different, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "WHOIS")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"WHOIS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The WHOIS print detailed information about an user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/whois user");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "AWAY")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"AWAY command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The AWAY set an away status or remove it");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The command with an argument set the status to the argument");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The command without any argument remove the status");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/away [reason]");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	else if(command_uper == "WALLOPS")
	{
		msg = Reply::RPL_HELPSTART_704(server,session,command_lower,"WALLOPS command");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On this server:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"The WALLOPS print a message on all users of the server");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"It doesnt need any ops privilege, yes its a bad idea");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"Usage:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"/wallops :message");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"On the RFC 2812:");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"different for obvious reasons, refer to rfc");
		msg += Reply::RPL_HELPTXT_705(server,session,command_lower,"");
	}
	msg += Reply::RPL_ENDOFHELP_706(server,session,command_lower,"End of /help");
	return(msg);
	
}


std::string	Command::info(Server *server, Session *session, Message  message)
{
	(void)message;
	if(session->getAuthenticated() == false)
		return ("");
	std::string msg;
	msg = Reply::RPL_INFO_371(server,session,"This server runs on the binary made for ft_irc");
	msg += Reply::RPL_INFO_371(server,session,"ft_irc is a 42school project that aim to");
	msg += Reply::RPL_INFO_371(server,session,"reproduce a fully functionnal irc serv with exceptions");
	msg += Reply::RPL_INFO_371(server,session,"");
	msg += Reply::RPL_INFO_371(server,session,"This server handle most of the commands,");
	msg += Reply::RPL_INFO_371(server,session,"except commands related to SASL, SSL, Server wide operator,");
	msg += Reply::RPL_INFO_371(server,session,"Multi server, Idle, ...");
	msg += Reply::RPL_INFO_371(server,session,"Type /help or /srvhelp for more details");
	msg += Reply::RPL_INFO_371(server,session,"Some implatation regardless hostname of clients can be");
	msg += Reply::RPL_INFO_371(server,session,"unusual, because this server and the clients are supose to be only local");
	msg += Reply::RPL_INFO_371(server,session,"");
	msg += Reply::RPL_INFO_371(server,session,"this project was done in aprox 2 weeks by:");
	msg += Reply::RPL_INFO_371(server,session,"edfirmin, ggualerz, ndesprez");
	msg += Reply::RPL_INFO_371(server,session,"this server was hosted briefly on irc.gmcg.fr, ggualerz domain");
	msg += Reply::RPL_ENDOFINFO_374(server,session);
	return(msg);
}

std::string	Command::mode(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.empty())
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	if(server->getChannel(message.params[0]) == NULL)
		return(Error::ERR_NOSUCHCHANNEL_403(server,session,message));
	if(message.params.size() == 1)
		return(Reply::RPL_CHANNELMODEIS_324(server,session,message) + Reply::RPL_CREATIONTIME_329(server,session,server->getChannel(message.params[0])));
	
	//Will go there if param[1] is populated
	if(message.params.size() >= 2 && (server->getChannel(message.params[0])->is_op(session->getNickName()) == false))
		return(Error::ERR_CHANOPRIVSNEEDED_482(server,session,message.params[0]));
	
	if(message.params.size() > 3)
		return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE should contain only one flag/arg pair"));
	if(message.params[1].size() != 2)
		return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE arg should contain maximum one operator and one flag"));
	if(message.params[1][0] != '+' && message.params[1][0] != '-')
		return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE should have as operator only + or -"));
	if(message.params[1][1] != 'i' && message.params[1][1] != 't' && message.params[1][1] != 'k' && message.params[1][1] != 'o' && message.params[1][1] != 'l')
		return(Error::ERR_UMODEUNKNOWNFLAG_501(server,session));
	
	Channel *chan = server->getChannel(message.params[0]);
	if(message.params[1][0] == '+' && (message.params[1][1] == 'i'))
	{
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +i should be called without an argument"));
		if(chan->get_inviteonly() == true)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +i cannot be called, chan already in +i"));
		chan->set_invite(true);
	}
	if(message.params[1][0] == '-' && (message.params[1][1] == 'i'))
	{
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -i should be called without an argument"));
		if(chan->get_inviteonly() == false)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -i cannot be called, chan already in -i"));
		chan->set_invite(false);
		chan->clear_invite(); //clear previous invites

	}
	if(message.params[1][0] == '+' && (message.params[1][1] == 't'))
	{
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +t should be called without an argument"));
		if(chan->get_topicrestricted() == true)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +t cannot be called, chan already in +t"));
		chan->set_op_topic(true);
	}
	if(message.params[1][0] == '-' && (message.params[1][1] == 't'))
	{
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -t should be called without an argument"));
		if(chan->get_topicrestricted() == false)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -t cannot be called, chan already in -t"));
		chan->set_op_topic(false);
	}
	if(message.params[1][0] == '+' && (message.params[1][1] == 'k'))
	{
		if(message.params[2].empty())
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +k should have the password as parameter"));
		else
			chan->set_pw(message.params[2]);
	}
	if(message.params[1][0] == '-' && (message.params[1][1] == 'k'))
	{
		if(chan->get_pw() == "")
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -k cannot be called, channel does not contain a password"));
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -k should be called without an argument"));
		chan->set_pw("");
	}
	if(message.params[1][0] == '+' && (message.params[1][1] == 'o'))
	{
		if(message.params[2].empty())
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +o should have a valid user as parameter"));
		else if(chan->is_op(message.params[2]))
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +o cannot op target already op"));
		else if(server->getSession(message.params[2]) == NULL)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +o should be called on an user connected on the server"));
		else
			chan->add_op(message.params[2]);
	}
	if(message.params[1][0] == '-' && (message.params[1][1] == 'o'))
	{
		if(message.params[2].empty())
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -o should have a valid user as parameter"));
		else if(!chan->is_op(message.params[2]))
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -o cannot deop someone not op"));
		else
			chan->rm_op(message.params[2]);
	}
	if(message.params[1][0] == '+' && (message.params[1][1] == 'l'))
	{
		
		if(message.params[2].empty())
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +l should contain a parameter represent the maximum user on this channel"));
		std::string tmp = message.params[2]; //I did this because atoi does crazy shit
		int max_user = atoi(tmp.c_str());
		if(max_user < 1 || max_user > 512)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE +l should be a number between 1 and 512 as parameter"));
		chan->set_max_users(max_user);
	}
	if(message.params[1][0] == '-' && (message.params[1][1] == 'l'))
	{
		if(chan->getMaxUsers() == 0)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -l cannot be called, channel already max_user is already unrestricted"));
		if(message.params.size() != 2)
			return(Error::ERR_INVALIDMODEPARAM_696(server,session,&message,"MODE -l should be called without an argument"));
		chan->set_max_users(0);
	}
	std::string to_chan;
	if(message.params.size() == 2)
		to_chan = Utils::getUserPrefix(server, session) + " MODE " + message.params[0] + " " + message.params[1] + Reply::endr;
	else
		to_chan = Utils::getUserPrefix(server, session) + " MODE " + message.params[0] + " " + message.params[1] + " " + message.params[2] +  Reply::endr;
	Utils::sendToChannel(server,chan,session->getNickName(), to_chan, message.params[0]);

	return(to_chan);
	
}



std::string	Command::whois(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.params.empty())
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	if(message.params.size() == 2)
		return(Error::ERR_NOSUCHSERVER_402(server,session,message));
	Session *target = server->getSession(message.params[0]);
	if(target == NULL)
		return(Error::ERR_NOSUCHNICK_401(server,session,message));
	std::string msg;
	// RPL_WHOISCERTFP (276). SSL certs not supported
	msg += Reply::RPL_WHOISREGNICK_307(server,session,target->getNickName());
	msg += Reply::RPL_WHOISUSER_311(server,session,target);
	msg += Reply::RPL_WHOISSERVER_312(server,session,target);
	// RPL_WHOISOPERATOR (313), server wide operator are not supported
	// RPL_WHOISIDLE (317), idle status is not supported
	msg += Reply::RPL_WHOISCHANNELS_319(server,session,target);
	msg += Reply::RPL_WHOISSPECIAL_320(server,session,target);
	// RPL_WHOISACCOUNT (330), SASL not supported
	// RPL_WHOISACTUALLY (338), Why bothering with that, all clients are localhost
	// RPL_WHOISHOST (378), Why bothering with that, all clients are localhost
	// RPL_WHOISMODES (379), no user mode supported
	// RPL_WHOISSECURE (671), SSL not supported
	if(target->getAwayStatus() != "")
		msg += Reply::RPL_AWAY_301(server,session,message);
	return(msg);
}


std::string	Command::away(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.payload.empty() && message.params.size() == 0)
	{
		if(session->getAwayStatus() != "")
		{
			session->setAwayStatus("");
			return(Reply::RPL_UNAWAY_305(server,session));
		}
		else
			return(""); //Ignore if user already un away
	}
	else
	{
		std::string away_status;
		if(!message.payload.empty())
			away_status = message.payload;
		else
			away_status = message.params[0];
		session->setAwayStatus(away_status);
		return(Reply::RPL_NOWAWAY_306(server,session));
	}
}

// std::string	Command::userhost(Server *server, Session *session, Message  message)
// {

	
// }

std::string	Command::wallops(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	if(message.payload.empty())
		return(Error::ERR_NEEDMOREPARAMS_461(server,session,message));
	
	std::map<int,Session *> all = server->getSessions();
	std::map<int,Session *>::iterator it = all.begin();
	std::string msg = "WALLOPS :" + message.payload + Reply::endr;
	while(it != all.end())
	{
		if(it->second != NULL)
			it->second->addSendBuffer(msg);
		it++;
	}
	return("");
}

std::string	Command::notimplanted(Server *server, Session *session, Message  message)
{
	if(session->getAuthenticated() == false)
		return ("");
	return(Error::ERR_NOPRIVILEGES_481(server,session,message));
}