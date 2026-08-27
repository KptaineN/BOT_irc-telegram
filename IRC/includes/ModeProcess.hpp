#ifndef MODEPROCESS_HPP
# define MODEPROCESS_HPP

# include <cstddef>
# include <string>
# include <vector>

class Server;
class Client;
class Channel;
struct IrcMessage;

class ModeProcess
{
	private:
		Server&					_server;
		Client&					_client;
		const IrcMessage&		_msg;
		Channel&				_channel;
		bool					_adding;
		std::size_t				_argumentIndex;
		std::string				_changes;
		std::vector<std::string>	_changeArguments;

		ModeProcess(const ModeProcess& other);
		ModeProcess& operator=(const ModeProcess& other);

		void	processQuery();
		void	processChanges();
		void	applySimpleMode(char mode);
		bool	handleKeyMode();
		bool	handleOperatorMode();
		bool	handleLimitMode();
		void	appendModeChange(bool adding, char mode);
		void	broadcastChanges();
		Client*	findClient(const std::string& nickname);
		bool	parseUserLimit(const std::string& value,
					std::size_t& limit) const;

	public:
		ModeProcess(Server& server, Client& client,
					const IrcMessage& msg, Channel& channel);
		~ModeProcess();

		void	execute();
};

#endif
