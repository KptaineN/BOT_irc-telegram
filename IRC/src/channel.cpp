#include "../includes/channel.hpp"
#include "../includes/server.hpp"

Channel::Channel(const std::string& name) 
    : _name(name), _topic(""), _activeModes(""), _key(""), _invitedUsers(), _userLimit(0)
{
}

Channel::~Channel()
{
}

const std::string& Channel::getName() const { return _name; }

const std::string& Channel::getTopic() const { return _topic; }

void Channel::setTopic(const std::string& topic) { _topic = topic; }

const std::map<int, Client*>& Channel::getMembers() const { return _members; }

void Channel::addClient(Client* client)
{
	if (client)
	{
		_members[client->getFd()] = client;
		if (_members.size() == 1)
			_operators[client->getFd()] = client;
	}
}

void Channel::removeClient(Client* client)
{
	if (client)
	{
		_members.erase(client->getFd());
		_operators.erase(client->getFd());

		if (_members.empty())
			return;

		if (_operators.empty())
		{
			std::map<int, Client*>::iterator it = _members.begin();
			if (it != _members.end())
			{
				addOperator(it->second);
			}
		}
	}
}

bool Channel::hasClient(int fd) const
{
	return (_members.find(fd) != _members.end());
}

void Channel::addOperator(Client* client)
{
	if (client && hasClient(client->getFd()))
		_operators[client->getFd()] = client;
}

void Channel::removeOperator(Client* client)
{
	if (client)
		_operators.erase(client->getFd());
}

bool Channel::isOperator(int fd) const
{
	return (_operators.find(fd) != _operators.end());
}

void Channel::addMode(char mode)
{
    // std::string::npos means the character was NOT found
    if (_activeModes.find(mode) == std::string::npos)
    {
        _activeModes += mode;
    }
}

void Channel::removeMode(char mode)
{
    size_t pos = _activeModes.find(mode);
    if (pos != std::string::npos)
    {
        _activeModes.erase(pos, 1);
    }
}

bool Channel::hasMode(char mode) const
{
    return (_activeModes.find(mode) != std::string::npos);
}

bool Channel::isInviteOnly() const
{
	return (hasMode('i'));
}

void Channel::broadcast(Server& server, const std::string& message, Client* sender)
{
	std::map<int, Client*>::iterator it = _members.begin();
	while (it != _members.end())
	{
		if (sender == NULL || it->second->getFd() != sender->getFd())
		{
			server.queueToClient(*(it->second), message);
		}
		++it;
	}
}

const std::string& Channel::getKey() const 
{ 
    return _key; 
}

void Channel::addInvitation(const std::string& nickname)
{
	if (!nickname.empty())
		_invitedUsers[nickname] = true;
}

void Channel::removeInvite(const std::string& nickname)
{
	_invitedUsers.erase(nickname);
}

bool Channel::isInvited(const std::string& nickname) const
{
	return (_invitedUsers.find(nickname) != _invitedUsers.end());
}

void Channel::setKey(const std::string& key) 
{ 
    _key = key; 
}

std::size_t Channel::getUserLimit() const
{
	return (_userLimit);
}

void Channel::setUserLimit(std::size_t limit_user_set)
{
	_userLimit = limit_user_set;
}

void Channel::renameInvite(const std::string& odlNickn,
	const std::string& newNickn)
{
	std::map<std::string, bool>::iterator it = _invitedUsers.find(odlNickn);
	if (it != _invitedUsers.end())
	{
		_invitedUsers.erase(it);
		_invitedUsers[newNickn] = true;
	}
}

std::string Channel::getNamesList() const {
    std::string list = "";
    std::map<int, Client*>::const_iterator it = _members.begin();
    while (it != _members.end()) {
        if (isOperator(it->first)) {
            list += "@";
        }
        list += it->second->getProfile().getNickname();
        ++it;
        if (it != _members.end()) {
            list += " ";
        }
    }
    return list;
}

bool Channel::isTopicRestricted() const
{
	return (hasMode('t'));
}
