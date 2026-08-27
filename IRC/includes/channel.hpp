#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <map>
# include <cstddef> // a checker
# include "client.hpp"

class Server;

class Channel
{
	private:
		std::string					_name;
		std::string					_topic;
		std::map<int, Client*>		_members;    // fd -> Client*
		std::map<int, Client*>		_operators;  // fd -> Client* (les @ops)
		std::string 				_activeModes;	// flag 'i' and 't' enable
		std::string                 _key;		// Stores password when 'k' is active
		std::map<std::string, bool>  _invitedUsers;	// invited nicknames for +i channels
        std::size_t                 _userLimit;

	public:
		Channel(const std::string& name);
		~Channel();

		// Getters
		const std::string&			getName() const;
		const std::string&			getTopic() const;
		void						setTopic(const std::string& topic);
		const std::map<int, Client*>& getMembers() const;

		// Users
		void						addClient(Client* client);
		void						removeClient(Client* client);
		bool						hasClient(int fd) const;

		bool                        hasMode(char mode) const;
		void                        addMode(char mode);
		void                        removeMode(char mode);
        
		// Key Management (For the 'k' mode)
        const std::string&          getKey() const;
        void                        setKey(const std::string& key);


		// Invite management (for +i channels)
		void                        addInvitation(const std::string& nickname);
		void                        removeInvite(const std::string& nickname);
		bool                        isInvited(const std::string& nickname) const;
        bool                        isInviteOnly() const;
        std::size_t                 getUserLimit() const;
        void                        setUserLimit(std::size_t limit_user_set); 
        void                        renameInvite(const std::string& odlNickn, const std::string& newNickn);

		std::string 				getNamesList() const;

		// Operator
		void						addOperator(Client* client);
		void						removeOperator(Client* client);
		bool						isOperator(int fd) const;
        bool                        isTopicRestricted() const;
		// Broadcast message
		void						broadcast(Server& server, const std::string& message, Client* sender = NULL);
};

#endif
