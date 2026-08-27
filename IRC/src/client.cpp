#include "../includes/client.hpp"

Client::Client() : _fd(-1), _inputBuffer(), _outputBuffer(), _profile(), _disconnectRequested(false), _quitReason("")
{
}

Client::Client(int fd) : _fd(fd), _inputBuffer(), _outputBuffer(), _profile(), _disconnectRequested(false), _quitReason("")
{
}

Client::~Client()
{
}

int	Client::getFd() const
{
	return (_fd);
}

ProfilIrc&	Client::getProfile()
{
	return (_profile);
}

const ProfilIrc&	Client::getProfile() const
{
	return (_profile);
}

void	Client::appendInput(const char *data, std::size_t len)
{
	_inputBuffer.append(data, len);
}

bool	Client::hasCommand() const
{
	return (_inputBuffer.find('\n') != std::string::npos);
}

std::string	Client::popCommand()
{
	std::size_t	pos;
	std::string	line;

	pos = _inputBuffer.find('\n');
	line = _inputBuffer.substr(0, pos);
	_inputBuffer.erase(0, pos + 1);

	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	return (line);
}

void	Client::appendOutput(const std::string& message)
{
	_outputBuffer += message;
}

bool	Client::hasOutput() const
{
	return (!_outputBuffer.empty());
}

std::string&	Client::getOutputBuffer()
{
	return (_outputBuffer);
}

void	Client::clearOutput(std::size_t count)
{
	if (count >= _outputBuffer.size())
		_outputBuffer.clear();
	else
		_outputBuffer.erase(0, count);
}

/*start Quentin_part*/
void	Client::requestDisconnect(const std::string& reason)
{
    _disconnectRequested = true;
    _quitReason = reason;
}
bool	Client::isDisconnectRequested() const
{
    return (_disconnectRequested);

}

const std::string& 	Client::getQuitReason() const
{
	return (_quitReason);
}
/*end Quentin_part*/