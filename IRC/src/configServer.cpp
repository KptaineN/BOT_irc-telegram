#include "../includes/configServer.hpp"

ServerConfig::ServerConfig()
	: _port(6667), _password(""), _serverName("ircserv")
{
}

ServerConfig::ServerConfig(int port, const std::string& password)
	: _port(port), _password(password), _serverName("ircserv")
{
}

ServerConfig::~ServerConfig()
{
}

int	ServerConfig::getPort() const
{
	return (_port);
}

const std::string&	ServerConfig::getPassword() const
{
	return (_password);
}

const std::string&	ServerConfig::getServerName() const
{
	return (_serverName);
}