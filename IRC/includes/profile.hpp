#ifndef PROFILE_HPP
# define PROFILE_HPP

# include <string>


class ProfilIrc
{
	private:
		std::string	_nickname;
		std::string	_username;
		std::string	_realname;

		bool		_passOk;
		bool		_hasNick;
		bool		_hasUser;
		bool		_registered;
		bool		_isOperator;

	public:
		ProfilIrc();
		~ProfilIrc();

		void				setPassOk(bool value);
		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);
		void				setRealname(const std::string& realname);

		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
		const std::string&	getRealname() const;

		bool				hasPass() const;
		bool				hasNick() const;
		bool				hasUser() const;
		bool				isRegistered() const;

		bool				canRegister() const;
		void				registerProfile();

		bool isServerOperator() const;
		void setServerOperator(bool op);
};

#endif