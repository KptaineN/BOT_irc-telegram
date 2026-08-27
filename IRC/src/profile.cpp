#include "../includes/profile.hpp"

ProfilIrc::ProfilIrc()
	: _passOk(false),
	    _hasNick(false),
	    _hasUser(false),
	    _registered(false)
{
}

ProfilIrc::~ProfilIrc()
{
}

void	ProfilIrc::setPassOk(bool value) {   _passOk = value;   }

void	ProfilIrc::setNickname(const std::string& nickname)
{
	_nickname = nickname;
	_hasNick = !_nickname.empty();
}

void	ProfilIrc::setRealname(const std::string& realname) {   _realname = realname;   }

void	ProfilIrc::setUsername(const std::string& username)
{
	_username = username;
	_hasUser = !_username.empty();
}

const std::string&	ProfilIrc::getNickname() const  {   return (_nickname); }

const std::string&	ProfilIrc::getUsername() const {    return (_username); }

const std::string&	ProfilIrc::getRealname() const  {   return (_realname); }

bool	ProfilIrc::hasPass() const  {   return (_passOk);   }

bool	ProfilIrc::hasNick() const  {   return (_hasNick);  }

bool	ProfilIrc::hasUser() const  {   return (_hasUser);  }

bool	ProfilIrc::isRegistered() const {   return (_registered);   }

bool	ProfilIrc::canRegister() const  {   return (_passOk && _hasNick && _hasUser);   }

void	ProfilIrc::registerProfile()
{
	if (canRegister())
		_registered = true;
}

bool ProfilIrc::isServerOperator() const { return _isOperator; }
void ProfilIrc::setServerOperator(bool op) { _isOperator = op; }