#include "../includes/server.hpp"
#include "../includes/debug.hpp"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cctype>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static std::string	toUpper(const std::string& str)
{
	std::string	result;
	std::size_t	i;

	result = str;
	i = 0;
	while (i < result.size())
	{
		result[i] = static_cast<char>(std::toupper(result[i]));
		++i;
	}
	return (result);
}

static IrcMessage	parseIrcLine(const std::string& line)
{
	IrcMessage			msg;
	std::string			rest;
	std::string			beforeTrailing;
	std::string			trailing;
	std::string			token;
	std::istringstream	iss;
	std::size_t			spacePos;
	std::size_t			trailingPos;

	rest = line;
	if (!rest.empty() && rest[0] == ':')
	{
		spacePos = rest.find(' ');
		if (spacePos == std::string::npos)
			return (msg);
		msg.prefix = rest.substr(1, spacePos - 1);
		rest.erase(0, spacePos + 1);
	}

	trailingPos = rest.find(" :");
	if (trailingPos != std::string::npos)
	{
		beforeTrailing = rest.substr(0, trailingPos);
		trailing = rest.substr(trailingPos + 2);
	}
	else
		beforeTrailing = rest;

	iss.str(beforeTrailing);
	if (!(iss >> msg.command))
		return (msg);

	if (msg.command.empty())
		return (msg);

	msg.command = toUpper(msg.command);

	while (iss >> token)
		msg.params.push_back(token);

	if (trailingPos != std::string::npos)
		msg.params.push_back(trailing);

	return (msg);
}

Server::Server(const ServerConfig& config)
	: _serverFd(-1), _config(config), _state(), _dispatcher()
{
	setupSocket();
}

/*start Quentin_part*/
// I change it to take on consideration the allacated channels
Server::~Server()
{
    std::size_t i = 0;
    while (i < _pollFds.size())
    {
        if (_pollFds[i].fd >= 0)
            close(_pollFds[i].fd);
        ++i;
    }

    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end())
    {
        if (it->second != NULL)
        {
            delete it->second;
        }
        ++it;
    }
    _channels.clear(); 
}

const std::map<std::string, Channel*>& Server::getChannels() const
{
    return _channels;
}
/*end Quentin_part*/


const ServerConfig&	Server::getConfig() const
{
	return (_config);
}

ServerState&	Server::getState()
{
	return (_state);
}

const CommandDispatcher& Server::getDispatcher() const
{
	return (_dispatcher);
}

void	Server::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl() failed");
}

void	Server::addPollFd(int fd)
{
	struct pollfd	pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
}

void	Server::enablePollOut(int fd)
{
	std::size_t	i;

	i = 0;
	while (i < _pollFds.size())
	{
		if (_pollFds[i].fd == fd)
		{
			_pollFds[i].events |= POLLOUT;
			return ;
		}
		++i;
	}
}

void	Server::disablePollOut(int fd)
{
	std::size_t	i;

	i = 0;
	while (i < _pollFds.size())
	{
		if (_pollFds[i].fd == fd)
		{
			_pollFds[i].events &= ~POLLOUT;
			return ;
		}
		++i;
	}
}

void	Server::setupSocket()
{
	struct sockaddr_in	addr;
	int					opt;

	DBG("Creating server socket...");
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
		throw std::runtime_error("socket() failed");

	opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR,
			&opt, sizeof(opt)) == -1)
		throw std::runtime_error("setsockopt() failed");

	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_config.getPort());

	DBG("Binding server on port " << _config.getPort());
	if (bind(_serverFd, reinterpret_cast<struct sockaddr *>(&addr),
			sizeof(addr)) == -1)
		throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno));

	if (listen(_serverFd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() failed");

	setNonBlocking(_serverFd);
	addPollFd(_serverFd);

	std::cout << BLUE << "Server listening on port "
		<< WHITE << _config.getPort() << RESET << std::endl;
}

void	Server::acceptNewClient()
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;
	int					clientFd;

	clientLen = sizeof(clientAddr);
	clientFd = accept(_serverFd,
			reinterpret_cast<struct sockaddr *>(&clientAddr),
			&clientLen);

	if (clientFd == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		std::cerr << RED << "accept() failed" << RESET << std::endl;
		return ;
	}

	setNonBlocking(clientFd);
	addPollFd(clientFd);
	_state.addClient(clientFd);

	std::cout << BLUE << "New client connected: fd "
		<< WHITE << clientFd << RESET << std::endl;
}

void	Server::queueToClient(Client& client, const std::string& message)
{
	std::string	data;

	data = message;
	if (data.size() < 2 || data.substr(data.size() - 2) != "\r\n")
		data += "\r\n";

	client.appendOutput(data);
	enablePollOut(client.getFd());
}

void	Server::reply(Client& client,
	const std::string& code,
	const std::string& message)
{
	std::string	nick;

	nick = client.getProfile().getNickname();
	if (nick.empty())
		nick = "*";

	queueToClient(client,
		":" + _config.getServerName() + " "
		+ code + " " + nick + " " + message);
}

bool	Server::receiveFromClient(std::size_t index)
{
	int			fd;
	char		buffer[512];
	ssize_t		bytes;
	Client*		client;

	fd = _pollFds[index].fd;
	bytes = recv(fd, buffer, sizeof(buffer), 0);
	DBG("recv_____________________________________________________________");

	if (bytes == 0)
	{
		removeClient(index);
		return (false);
	}
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		removeClient(index);
		return (false);
	}

	client = _state.getClientByFd(fd);
	if (client == NULL)
		return (true);

	client->appendInput(buffer, static_cast<std::size_t>(bytes));

	while (client->hasCommand())
	{
		std::string	line;

		line = client->popCommand();
		processClientCommand(*client, line);
		/*start Quentin_part*/
		if (client->isDisconnectRequested())
		{
			removeClient(index);
			return (false);
		}
		/*end Quentin_part*/
	}

	return (true);
}

bool	Server::sendToClient(std::size_t index)
{
	int			fd;
	ssize_t		bytes;
	Client*		client;

	fd = _pollFds[index].fd;
	client = _state.getClientByFd(fd);
	if (client == NULL)
		return (true);

	std::string&	output = client->getOutputBuffer();

	if (output.empty())
	{
		disablePollOut(fd);
		return (true);
	}

	bytes = send(fd, output.c_str(), output.size(), 0);
	DBG("send____________________________________________________________");
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		removeClient(index);
		return (false);
	}

	client->clearOutput(static_cast<std::size_t>(bytes));
	if (!client->hasOutput())
		disablePollOut(fd);

	return (true);
}

void	Server::removeClient(std::size_t index)
{
	int	fd;
	Client *client;

	fd = _pollFds[index].fd;
	client = _state.getClientByFd(fd);
	
	if (client != NULL)
	{
		const std::string nick = client->getProfile().getNickname();
		const std::string user = client->getProfile().getUsername();
		std::map<std::string, Channel*>::iterator it = _channels.begin();

		while (it != _channels.end())
		{
			Channel *channel = it->second;
			if (channel != NULL)
			{
				if (channel->hasClient(fd))
				{
					channel->broadcast(*this,
						":" + nick + "!" + user
						+ "@localhost QUIT :Client Quit", client);
					channel->removeClient(client);
				}
				channel->removeInvite(nick);
			}
			if (channel == NULL || channel->getMembers().empty())
			{
				delete channel;
				_channels.erase(it++);
			}
			else
				++it;
		}
	}
	
	DBG("Client disconnected, fd = " << fd);
	std::cout << RED << "Client disconnected: fd "
		<< WHITE << fd << RESET << std::endl;

	close(fd);
	_state.removeClient(fd);
	_pollFds.erase(_pollFds.begin() + index);
}

void	Server::processClientCommand(Client& client, const std::string& line)
{
	IrcMessage	msg;

	if (line.empty())
		return ;

	std::cout << "[client fd " << client.getFd() << "] ";
	std::cout << "command received: \"" << line << "\"" << std::endl;

	msg = parseIrcLine(line);

	if (msg.command.empty())
		return ;

	_dispatcher.dispatch(*this, client, msg);
}

/*start Quentin_part*/
Channel* Server::getOrCreateChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end()) {
        return it->second;
    }
    Channel* newChan = new Channel(name);
    _channels[name] = newChan;
    return newChan;
}
/*end Quentin_part*/

Channel* Server::getChannel(const std::string& name) const {
	std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

void Server::removeChannelIfEmpty(const std::string& name) {
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it != _channels.end()) {
		if (it->second->getMembers().empty()) {
			delete it->second;
			_channels.erase(it);
		}
	}
}

void	Server::run()
{
	while (true)
	{
		int	ret;

		ret = poll(&_pollFds[0], _pollFds.size(), -1);
		if (ret == -1)
		{
			if (errno == EINTR)
				continue ;
			throw std::runtime_error("poll() failed");
		}

		for (std::size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents == 0)
				continue ;

			if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				if (_pollFds[i].fd == _serverFd)
					throw std::runtime_error("server socket error");
				removeClient(i);
				--i;
				continue ;
			}

			if (_pollFds[i].revents & POLLIN)
			{
				if (_pollFds[i].fd == _serverFd)
					acceptNewClient();
				else if (!receiveFromClient(i))
				{
					--i;
					continue ;
				}
			}

			if (i < _pollFds.size() && (_pollFds[i].revents & POLLOUT))
			{
				if (!sendToClient(i))
					--i;
			}
		}
	}
}