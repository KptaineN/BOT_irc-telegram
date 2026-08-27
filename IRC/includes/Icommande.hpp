#ifndef ICOMMANDE_HPP
# define ICOMMANDE_HPP

# include <map>
# include <string>
# include <vector>
# include <sstream>
#include <fstream> //for motd

struct IrcMessage
{
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;
};

class Server;
class Client;

class ICommand
{
	public:
		virtual ~ICommand() {}
		virtual void execute(Server& server, Client& client, const IrcMessage& msg) = 0;
};

class PassCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class NickCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class UserCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class PingCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

/*start Quentin_part*/
class PrivmsgCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class QuitCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class WhoisCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class HelpCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class MotdCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class JoinCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class PartCommand : public ICommand
{
    public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class KickCommand : public ICommand
{
	public:
		void execute(Server& server, Client& client, const IrcMessage& msg);
};

class NamesCommand : public ICommand
{
    public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class ListCommand : public ICommand
{
	public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class KillCommand : public ICommand
{
	public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class InviteCommand : public ICommand
{
	public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class ModeCommand : public ICommand
{
    public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class TopicCommand : public ICommand
{
    public:
        void execute(Server& server, Client& client, const IrcMessage& msg);
};

class CommandDispatcher
{
	private:
		std::map<std::string, ICommand*>	_commands;

		CommandDispatcher(const CommandDispatcher& other);
		CommandDispatcher& operator=(const CommandDispatcher& other);

	public:
		CommandDispatcher();
		~CommandDispatcher();

		void dispatch(Server& server, Client& client, const IrcMessage& msg);
		const std::map<std::string, ICommand*>& getCommands() const;
};

#endif
