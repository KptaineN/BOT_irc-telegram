#include "../includes/stateServer.hpp"

ServerState::ServerState()
{
}

ServerState::~ServerState()
{
}

std::map<int, Client>&	ServerState::getClients()
{
	return (_clients);
}

std::map<std::string, Channel>&	ServerState::getChannels()
{
	return (_channels);
}

Client*	ServerState::getClientByFd(int fd)
{
	std::map<int, Client>::iterator	it;

	it = _clients.find(fd);
	if (it == _clients.end())
		return (NULL);
	return (&it->second);
}

Client*	ServerState::getClientByNickname(const std::string& nickname)
{
	std::map<int, Client>::iterator	it;

	it = _clients.begin();
	while (it != _clients.end())
	{
		if (it->second.getProfile().getNickname() == nickname)
			return (&it->second);
		++it;
	}
	return (NULL);
}

bool	ServerState::isNicknameUsed(const std::string& nickname, int exceptFd) const
{
	std::map<int, Client>::const_iterator	it;

	it = _clients.begin();
	while (it != _clients.end())
	{
		if (it->first != exceptFd
			&& it->second.getProfile().getNickname() == nickname)
			return (true);
		++it;
	}
	return (false);
}

void	ServerState::addClient(int fd)
{
	_clients.insert(std::make_pair(fd, Client(fd)));
}

void	ServerState::removeClient(int fd)
{
	_clients.erase(fd);
}