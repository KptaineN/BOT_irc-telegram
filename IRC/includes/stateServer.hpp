#ifndef STATESERVER_HPP
# define STATESERVER_HPP

# include "client.hpp"
# include "channel.hpp"
# include <map>
# include <string>

class ServerState
{
	private:
		std::map<int, Client>			_clients;
		std::map<std::string, Channel>	_channels;

	public:
		ServerState();
		~ServerState();

		std::map<int, Client>&			getClients();
		std::map<std::string, Channel>&	getChannels();

		Client*							getClientByFd(int fd);
		Client*							getClientByNickname(const std::string& nickname);

		bool							isNicknameUsed(const std::string& nickname, int exceptFd) const;

		void							addClient(int fd);
		void							removeClient(int fd);
};

#endif