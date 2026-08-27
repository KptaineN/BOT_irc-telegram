#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <cstddef>

# include "profile.hpp"


//représente une connexion TCP acceptée. user se connecte avec nc, HexChat, irssi, WeeChat, etc., le serveur va créer un objet Client.
class Client
{   
    
    private:
		int			_fd;
		std::string	_inputBuffer;
		std::string	_outputBuffer;
		ProfilIrc	_profile;
		/*start Quentin_part*/
		bool _disconnectRequested;
		std::string _quitReason;
		/*end Quentin_part*/

	public:
		Client();
		Client(int fd);
		~Client();

		int					getFd() const;

		ProfilIrc&			getProfile();
		const ProfilIrc&	getProfile() const;

		void				appendInput(const char *data, std::size_t len);
		bool				hasCommand() const;
		std::string			popCommand();

		void				appendOutput(const std::string& message);
		bool				hasOutput() const;
		std::string&		getOutputBuffer();
		void				clearOutput(std::size_t count);
		/*start Quentin_part*/
		void				requestDisconnect(const std::string& reason);
		bool 				isDisconnectRequested() const;
		const std::string& 	getQuitReason() const;
		/*end Quentin_part*/
    
    
    
    
    
    /*
	private:
		int			_fd;
		std::string	_buffer;

	public:
		Client() : _fd(-1) {}; //-1 = Client invalide / pas encore connecté
		Client(int fd) : _fd(fd) {};
		~Client() {};

		int					getFd() const {return (_fd);}; //renvoie le fd du client.
                //data = données brutes reçues par recv(). len = nombre exact de caractères reçus. 
		void				appendBuffer(const char *data, std::size_t len){
            _buffer.append(data, len);
        };
		bool				hasCommand() const{
            return (_buffer.find('\n') != std::string::npos); //ligne qui contient \n. pour cmd complete
        }
		std::string			popCommand(){
                std::size_t	pos;    //va contenir la position du \n.
                std::string	line;   //va contenir la commande extraite.

                pos = _buffer.find('\n');
                line = _buffer.substr(0, pos); //prends tout depuis le début du buffer jusqu’avant le \n
                _buffer.erase(0, pos + 1);    //supprimes du buffer la commande qui viens d’etre extrait. pos + 1 pour enlever aussi le \n

                if (!line.empty() && line[line.size() - 1] == '\r') //si le dernier caractère est \r.
                    line.erase(line.size() - 1);                    //on le suprime

                return (line); //renvoies la commande
        };*/
        /*
        Cette fonction récupère une commande complète depuis le buffer, puis la supprime du buffer.
            Exemple :
                _buffer = "PASS secret\r\nNICK noe\r\n"
            Après popCommand() :
                retour = "PASS secret"
                _buffer = "NICK noe\r\n"
        */
};

#endif