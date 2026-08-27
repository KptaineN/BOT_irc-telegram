#ifndef CONFIGSERVER_HPP
# define CONFIGSERVER_HPP

# include <string>

class ServerConfig
{
	private:
		int			_port;
		std::string	_password;
		std::string	_serverName;

	public:
		ServerConfig();
		ServerConfig(int port, const std::string& password);
		~ServerConfig();

		int					getPort() const;
		const std::string&	getPassword() const;
		const std::string&	getServerName() const;
};

#endif

/*
contient les infos fixes :
                        -     port
                        -     password
                        -     server name
                        -     date de création éventuellement
                        -     limite de clients éventuellement
*/                      