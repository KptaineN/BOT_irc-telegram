#ifndef SERVER_HPP
# define SERVER_HPP

# include <string> // string for _password
# include <vector> //vector for stock pollfd on _fds
# include <map> //pour associer un fd à un _clients
# include <poll.h> //for struct pollfd
# include "channel.hpp"

# include "client.hpp"
# include "configServer.hpp"
# include "stateServer.hpp"
# include "Icommande.hpp"

# define C_FORM  "\033[30m"
# define YELLOW "\033[33m"
# define RRED "\033[31m"
# define RED "\033[91m"
# define GREEN "\033[92m"
# define WHITE "\033[37m"
# define RESET "\033[0m"
# define B_RED "\033[1;4;91m"
# define BLUE "\033[94m"
# define MAGENTA "\033[1;95m"
# define SLG "\033[4m"

class Server
{
	private:
		int									_serverFd;
		std::vector<struct pollfd>			_pollFds;
		
		ServerConfig						_config;
		ServerState							_state;
		CommandDispatcher					_dispatcher;
		std::map<std::string, Channel*>		_channels;

		Server();
		Server(const Server& other);
		Server&	operator=(const Server& other);

	public:
		Server(const ServerConfig& config);
		~Server();

		void				run();

		const ServerConfig&	getConfig() const;
		ServerState&		getState();
		const CommandDispatcher& getDispatcher() const;
		Channel* 			getOrCreateChannel(const std::string& name);
		Channel* 			getChannel(const std::string& name) const;
		void				removeChannelIfEmpty(const std::string& name);
		const std::map<std::string, Channel*>& getChannels() const;

		void				reply(Client& client,
								const std::string& code,
								const std::string& message);

		void				queueToClient(Client& client,
								const std::string& message);

	private:
		void				setupSocket();
		void				setNonBlocking(int fd);
		void				addPollFd(int fd);

		void				enablePollOut(int fd);
		void				disablePollOut(int fd);

		void				acceptNewClient();
		bool				receiveFromClient(std::size_t index);
		bool				sendToClient(std::size_t index);
		void				removeClient(std::size_t index);

		void				processClientCommand(Client& client,
								const std::string& line);
};

#endif