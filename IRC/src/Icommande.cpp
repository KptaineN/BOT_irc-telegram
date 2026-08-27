#include "../includes/Icommande.hpp"
#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include <cctype>
#include <limits>
#include <sstream>

static std::string	toLowerAscii(const std::string& input)
{
	std::string	result;

	result.reserve(input.size());
	for (std::string::const_iterator it = input.begin(); it != input.end(); ++it)
		result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*it))));
	return (result);
}

static void	tryRegister(Server& server, Client& client)
{
	ProfilIrc&	profile = client.getProfile();

	if (profile.isRegistered())
		return ;

	if (profile.canRegister())
	{
		profile.registerProfile();
		server.reply(client, "001", ":Welcome to ft_irc " + profile.getNickname());
		/*start Quentin_part*/
		MotdCommand motd;
        IrcMessage emptyMsg;
		motd.execute(server, client, emptyMsg);
		/*end Quentin_part*/

	}
}

CommandDispatcher::CommandDispatcher()
{
	_commands["PASS"] = new PassCommand();
	_commands["NICK"] = new NickCommand();
	_commands["USER"] = new UserCommand();
	_commands["PING"] = new PingCommand();
	_commands["PRIVMSG"] = new PrivmsgCommand();
	_commands["QUIT"] = new QuitCommand();
	_commands["WHOIS"] = new WhoisCommand();
	_commands["HELP"] = new HelpCommand();
	_commands["JOIN"] = new JoinCommand();
	_commands["PART"] = new PartCommand();
	_commands["KICK"] = new KickCommand();
	_commands["NAMES"] = new NamesCommand();
	_commands["LIST"] = new ListCommand();
	_commands["KILL"] = new KillCommand();
	_commands["INVITE"] = new InviteCommand();
	_commands["MODE"] = new ModeCommand();
	_commands["TOPIC"] = new TopicCommand();
}

CommandDispatcher::~CommandDispatcher()
{
	std::map<std::string, ICommand*>::iterator	it;

	it = _commands.begin();
	while (it != _commands.end())
	{
		delete it->second;
		++it;
	}
}

void	CommandDispatcher::dispatch(Server& server, Client& client, const IrcMessage& msg)
{
	std::map<std::string, ICommand*>::iterator	it;

	it = _commands.find(msg.command);
	if (it == _commands.end())
	{
		server.reply(client, "421", msg.command + " :Unknown command");
		return ;
	}

	it->second->execute(server, client, msg);
}

/*start Quentin_part*/
const std::map<std::string, ICommand*>& CommandDispatcher::getCommands() const
{
    return _commands;
}
/*end Quentin_part*/

void	PassCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (client.getProfile().isRegistered())
	{
		server.reply(client, "462", ":You may not reregister");
		return ;
	}

	if (msg.params.empty())
	{
		server.reply(client, "461", "PASS :Not enough parameters");
		return ;
	}

	if (msg.params[0] != server.getConfig().getPassword())
	{
		server.reply(client, "464", ":Password incorrect");
		return ;
	}

	client.getProfile().setPassOk(true);
	tryRegister(server, client);
}

void	NickCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (msg.params.empty() || msg.params[0].empty())
	{
		server.reply(client, "431", ":No nickname given");
		return ;
	}

	if (server.getState().isNicknameUsed(msg.params[0], client.getFd()))
	{
		server.reply(client, "433", msg.params[0] + " :Nickname is already in use");
		return ;
	}
	std::string name2rename = client.getProfile().getNickname();
	std::string newNickname = msg.params[0];
	client.getProfile().setNickname(newNickname);
	if (!name2rename.empty() && name2rename != newNickname)
	{
		const std::map<std::string, Channel*>& channels = server.getChannels();
		std::map<std::string, Channel*>::const_iterator it = channels.begin();
		while (it != channels.end())
		{
			if (it->second != NULL)
				it->second->renameInvite(name2rename, newNickname);
			++it;
		}
	}
	tryRegister(server, client);
}

void	UserCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (client.getProfile().isRegistered())
	{
		server.reply(client, "462", ":You may not reregister");
		return ;
	}

	if (msg.params.size() < 4)
	{
		server.reply(client, "461", "USER :Not enough parameters");
		return ;
	}

	client.getProfile().setUsername(msg.params[0]);
	client.getProfile().setRealname(msg.params[3]);

	tryRegister(server, client);
}

void	PingCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (msg.params.empty())
	{
		server.reply(client, "409", ":No origin specified");
		return ;
	}

	server.queueToClient(client,
		":" + server.getConfig().getServerName()
		+ " PONG " + server.getConfig().getServerName()
		+ " :" + msg.params[0]);
}

/*start Quentin_part*/
void	PrivmsgCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (!client.getProfile().isRegistered())
	{
		server.reply(client, "451", ":You have not registered");
		return;
	}
	if (msg.params.empty())
	{
		server.reply(client, "411", ": No recipient given (PRIVMSG)");
		return;
	}
	if (msg.params.size() < 2)
	{
		server.reply(client, "412", ": No text to send");
		return;
	}
	std::string target_nickname = msg.params[0];
	std::string message = msg.params[1];
	std::string sender_nickname = client.getProfile().getNickname();

	if (!target_nickname.empty() && (target_nickname[0] == '#' || target_nickname[0] == '&'))
	{
		Channel* channel = server.getOrCreateChannel(target_nickname);
	
		if (!channel->hasClient(client.getFd()))
		{
			server.reply(client, "442", target_nickname + " :You're not on that channel");
			return;
		}

		std::string user = client.getProfile().getUsername();
		std::string formattedMsg = ":" + sender_nickname + "!" + user + "@localhost PRIVMSG " + target_nickname + " :" + message;

		// Broadcast to everyone in the channel EXCEPT the sender
		channel->broadcast(server, formattedMsg, &client);
		return;
	}

	Client* target_client = server.getState().getClientByNickname(target_nickname);

	if(target_client == NULL)
	{
		server.reply(client, "401", ": No such nickname/channel");
		return;
	}

	server.queueToClient(*target_client,
    	":" + sender_nickname + " PRIVMSG " + target_nickname + " :" + message);
}

void	QuitCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	(void) server; //changer plus tard, je le void pour eviter les erreurs potentiel de compil.
	std::string quit_message = "";
	if (msg.params.empty())
		quit_message = "Client Quit";
	else
		quit_message = msg.params[0];
	client.requestDisconnect(quit_message);
}

void	WhoisCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (!client.getProfile().isRegistered())
	{
    	server.reply(client, "451", ":You have not registered");
    	return;
	}
	if (msg.params.empty())
	{
		server.reply(client, "461", "WHOIS :Not enough parameters");
		return;
	}
	std::string target_nickname = msg.params[0];
	Client* target_client = server.getState().getClientByNickname(target_nickname);
	if (target_client == NULL)
	{
		server.reply(client, "401", target_nickname + " :No such nick/channel");
		return;
	}
	std::string info = target_nickname + " " + target_client->getProfile().getUsername() + " * :" + 
	target_client->getProfile().getRealname();
	server.reply(client, "311", info);
	server.reply(client, "318", target_nickname + " :End of WHOIS list");
}

void	HelpCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    (void)msg;

    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }
	const std::map<std::string, ICommand*>& commands = server.getDispatcher().getCommands();
	server.reply(client, "704", ":Available commands:");
	for (std::map<std::string, ICommand*>::const_iterator it = commands.begin(); it != commands.end(); ++it)
	{
    	server.reply(client, "704", ":" + it->first);
	}
	server.reply(client, "706", ":End of HELP list");
}

void	JoinCommand::execute(Server& server, Client& client, const IrcMessage& msg) {

	if (!client.getProfile().isRegistered()) {
            server.reply(client, "451", ":You have not registered");
            return;
        }

        if (msg.params.empty()) {
            server.reply(client, "461", "JOIN :Not enough parameters");
            return;
        }

        std::string chanName = msg.params[0];
        std::string key;

		if(msg.params.size() > 1)
			key = msg.params[1];
		else
			key = "";

        if (chanName.empty() || (chanName[0] != '#' && chanName[0] != '&')) {
            server.reply(client, "476", chanName + " :Bad Channel Mask");
            return;
        }

		// 2. Fetch or create the channel
    	bool isNewChannel = (server.getOrCreateChannel(chanName)->getMembers().empty());
   		
		Channel* channel  = server.getChannel(chanName);
		
		isNewChannel = (channel == NULL);

		if (isNewChannel)
        	channel = server.getOrCreateChannel(chanName);

        // 3. Skip mode verification if the user is already inside
        if (channel->hasClient(client.getFd())) {
            return; 
        }

		if (isNewChannel && !key.empty()) {
        	channel->addMode('k');
        	channel->setKey(key);
    	}
        // 4. Validate Key Flag (+k)
		// 4.a & b
		if (channel->isInviteOnly() && !channel->isInvited(client.getProfile().getNickname())){
			server.reply(client, "473", chanName + " :Cannot join channel (+i)");
			return;
		}
		if (channel->hasMode('l')
			&& channel->getMembers().size() >= channel->getUserLimit()){
			server.reply(client, "471", chanName + " :Cannot join channel (+l)");
			return;
		}
		
		if (channel->hasMode('k') && channel->getKey() != key) {
            server.reply(client, "475", chanName + " :Cannot join channel (+k)");
            return;
        }



        // 5. Add user to channel (automatically updates operator status if they are first)
        channel->addClient(&client);

        // 6. Broadcast successful join to everyone in the channel (including sender)
        // Format required by IRC clients: :Nick!User@Host JOIN :#channel
        //std::string nick = client.getProfile().getNickname();
        //std::string user = client.getProfile().getUsername();
        //std::string joinNotification = ":" + nick + "!" + user + "@localhost JOIN :" + chanName;
        
        // Passing NULL as the 3rd param ensures the sender receives their own join validation
       // channel->broadcast(server, joinNotification, NULL);
		channel->broadcast(server,
			":" + client.getProfile().getNickname() + "!"
			+ client.getProfile().getUsername() + "@localhost JOIN :" + chanName, NULL);
	
        // 7. Send Topic status if it exists
        if (!channel->getTopic().empty()) {
            server.reply(client, "332", chanName + " :" + channel->getTopic());
        }

        // 8. Send Names list replies (RPL_NAMREPLY 353 & RPL_ENDOFNAMES 366)
        server.reply(client, "353", "= " + chanName + " :" + channel->getNamesList());
        server.reply(client, "366", chanName + " :End of /NAMES list");
};

void	PartCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (!client.getProfile().isRegistered())
	{
		server.reply(client, "451", ":You have not registered");
		return;
	}

	if (msg.params.empty())
	{
		server.reply(client, "461", "PART :Not enough parameters");
		return;
	}

	std::string chanName = msg.params[0];
	std::string reason;
	if (msg.params.size() > 1)
		reason = msg.params[1];

	if (chanName.empty() || (chanName[0] != '#' && chanName[0] != '&'))
	{
		server.reply(client, "403", chanName + " :No such channel");
		return;
	}

	Channel* channel = server.getChannel(chanName);
	if (channel == NULL)
	{
		server.reply(client, "403", chanName + " :No such channel");
		return;
	}

	if (!channel->hasClient(client.getFd()))
	{
		server.reply(client, "442", chanName + " :You're not on that channel");
		return;
	}

	std::string nick = client.getProfile().getNickname();
	std::string user = client.getProfile().getUsername();
	std::string partNotification;
	if (!reason.empty())
		partNotification = ":" + nick + "!" + user + "@localhost PART " + chanName + " :" + reason;
	else
		partNotification = ":" + nick + "!" + user + "@localhost PART " + chanName;

	channel->broadcast(server, partNotification, NULL);
	channel->removeClient(&client);

	server.removeChannelIfEmpty(chanName);
}

void	KickCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
	if (!client.getProfile().isRegistered())
	{
		server.reply(client, "451", ":You have not registered");
		return;
	}

	if (msg.params.size() < 2)
	{
		server.reply(client, "461", "KICK :Not enough parameters");
		return;
	}

	std::string chanName = msg.params[0];
	std::string targetNick = msg.params[1];
	std::string reason;
	if (msg.params.size() > 2)
		reason = msg.params[2];

	if (chanName.empty() || (chanName[0] != '#' && chanName[0] != '&'))
	{
		server.reply(client, "403", chanName + " :No such channel");
		return;
	}

	Channel* channel = server.getChannel(chanName);
	if (channel == NULL)
	{
		server.reply(client, "403", chanName + " :No such channel");
		return;
	}

	if (!channel->hasClient(client.getFd()))
	{
		server.reply(client, "442", chanName + " :You're not on that channel");
		return;
	}

	if (!channel->isOperator(client.getFd()))
	{
		server.reply(client, "482", chanName + " :You're not channel operator");
		return;
	}

	Client* targetClient = server.getState().getClientByNickname(targetNick);
	if (targetClient == NULL)
	{
		std::string	lowerTargetNick = toLowerAscii(targetNick);
		std::map<int, Client>& clients = server.getState().getClients();

		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (toLowerAscii(it->second.getProfile().getNickname()) == lowerTargetNick)
			{
				targetClient = &it->second;
				break;
			}
		}
	}
	if (targetClient == NULL)
	{
		server.reply(client, "401", targetNick + " :No such nick/channel");
		return;
	}

	if (!channel->hasClient(targetClient->getFd()))
	{
		server.reply(client, "441", targetNick + " " + chanName + " :They aren't on that channel");
		return;
	}

	std::string nick = client.getProfile().getNickname();
	std::string user = client.getProfile().getUsername();
	std::string targetNickCanonical = targetClient->getProfile().getNickname();
	std::string kickMessage = ":" + nick + "!" + user + "@localhost KICK " + chanName + " " + targetNickCanonical;
	if (!reason.empty())
		kickMessage += " :" + reason;

	channel->broadcast(server, kickMessage, NULL);
	channel->removeClient(targetClient);
	server.removeChannelIfEmpty(chanName);
}

void	NamesCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }

    if (msg.params.empty())
    {
        server.reply(client, "461", "NAMES :Not enough parameters");
        return;
    }

    std::string chanName = msg.params[0];
    Channel* channel = server.getChannel(chanName);

    if (channel == NULL)
    {
        server.reply(client, "403", chanName + " :No such channel");
        return;
    }

    server.reply(client, "353", "= " + chanName + " :" + channel->getNamesList());
    server.reply(client, "366", chanName + " :End of /NAMES list");
}

void	ListCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    (void)msg;

    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }

    const std::map<std::string, Channel*>& channels = server.getChannels();

    server.reply(client, "321", "Channel :Users Name");

    for (std::map<std::string, Channel*>::const_iterator it = channels.begin();
         it != channels.end(); ++it)
    {
        Channel* channel = it->second;
        if (!channel)
            continue;

        std::ostringstream oss;
        oss << channel->getName() << " " << channel->getMembers().size();
        if (!channel->getTopic().empty())
            oss << " :" << channel->getTopic();

        server.reply(client, "322", oss.str());
    }

    server.reply(client, "323", ":End of /LIST list");
}

void KillCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    if (msg.params.size() < 2)
    {
        server.reply(client, "461", "KILL :Not enough parameters");
        return;
    }

    std::string targetNick = msg.params[0];
    std::string reason = msg.params[1];

    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }

	if (!client.getProfile().isServerOperator())
	{
    	server.reply(client, "481", ":Permission Denied - You're not an Operator");
    	return;
	}

    Client* targetClient = server.getState().getClientByNickname(targetNick);
    if (targetClient == NULL)
    {
        server.reply(client, "401", targetNick + " :No such nick/channel");
        return;
    }

    std::string killMessage = ":" + client.getProfile().getNickname() + "!" 
        + client.getProfile().getUsername() + "@localhost KILL " + targetNick + " :" + reason;

    server.queueToClient(*targetClient, killMessage);
    targetClient->requestDisconnect(reason);
}


//je dois encore attendre pour les modes pour verifier le +i (invitaion channel)
void InviteCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }

    if (msg.params.size() < 2)
    {
        server.reply(client, "461", "INVITE :Not enough parameters");
        return;
    }

    std::string targetNick = msg.params[0];
    std::string chanName = msg.params[1];

    Client* targetClient = server.getState().getClientByNickname(targetNick);
    if (targetClient == NULL)
    {
        server.reply(client, "401", targetNick + " :No such nick/channel");
        return;
    }

    Channel* channel = server.getChannel(chanName);
    if (channel == NULL)
    {
        server.reply(client, "403", chanName + " :No such channel");
        return;
    }

    if (!channel->hasClient(client.getFd()))
    {
        server.reply(client, "442", chanName + " :You're not on that channel");
        return;
    }

    if (channel->hasClient(targetClient->getFd()))
    {
        server.reply(client, "443", targetNick + " " + chanName + " :is already on channel");
        return;
    }
    if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
    {
        server.reply(client, "482", chanName + " :You're not channel operator");
        return;
    }

    channel->addInvitation(targetClient->getProfile().getNickname());

    server.reply(client, "341", targetNick + " " + chanName);

    std::string nick = client.getProfile().getNickname();
    std::string user = client.getProfile().getUsername();
    std::string inviteMsg = ":" + nick + "!" + user + "@localhost INVITE " + targetNick + " " + chanName;
    server.queueToClient(*targetClient, inviteMsg);
}

void MotdCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    (void)msg;
    
    server.reply(client, "375", ":- " + server.getConfig().getServerName() + " Message of the Day -");

    std::ifstream file("motd.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            server.reply(client, "372", ":- " + line);
        }
        file.close();
    }
    else
    {
        server.reply(client, "372", ":- Welcome to ft_irc! (motd.txt missing)");
    }
    server.reply(client, "376", ":End of /MOTD command.");
}
/*end Quentin_part*/


void TopicCommand::execute(Server& server, Client& client, const IrcMessage& msg)
{
    if (!client.getProfile().isRegistered())
    {
        server.reply(client, "451", ":You have not registered");
        return;
    }

    if (msg.params.empty())
    {
        server.reply(client, "461", "TOPIC :Not enough parameters");
        return;
    }

    std::string chanName = msg.params[0];
    Channel* channel = server.getChannel(chanName);

    if (channel == NULL)
    {
        server.reply(client, "403", chanName + " :No such channel");
        return;
    }

    if (!channel->hasClient(client.getFd()))
    {
        server.reply(client, "442", chanName + " :You're not on that channel");
        return;
    }

    if (msg.params.size() == 1)
    {
        if (channel->getTopic().empty())
        {
            server.reply(client, "331", chanName + " :No topic is set");
        }
        else
        {
            server.reply(client, "332", chanName + " :" + channel->getTopic());
        }
        return;
    }
    
    if (channel->isTopicRestricted() && !channel->isOperator(client.getFd()))
    {
        server.reply(client, "482", chanName + " :You're not channel operator");
        return;
    }

    std::string newTopic = msg.params[1];
    channel->setTopic(newTopic);

    std::string nick = client.getProfile().getNickname();
    std::string user = client.getProfile().getUsername();
    std::string topicMsg = ":" + nick + "!" + user + "@localhost TOPIC " + chanName + " :" + newTopic;
    
    channel->broadcast(server, topicMsg, NULL);
}
