#include "../includes/server.hpp"

#include <iostream> //Pour std::cerr
#include <cstdlib> //Pour std::strtol.
#include <cerrno> //Pour errno
#include <stdexcept> //std::runtime_error

//vérifie si une string contient uniquement des chiffres.
static bool	isNumber(const std::string &str)
{
	if (str.empty())
		return (false);

	for (std::size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] < '0' || str[i] > '9')
			return (false);
	}
	return (true);
}
//fonction convertit une string en port valide.
static int	parsePort(const std::string &str)
{
	char	*end;
	long	port;

	if (!isNumber(str)) //refuses toute valeur non numérique.
		throw std::runtime_error("invalid port");

	errno = 0; //Parce que errno est une variable globale qui peut contenir une ancienne erreur
	port = std::strtol(str.c_str(), &end, 10); //utiliser pour dire où la conversion s’est arrêtée.
	//convertis la string en nombre.
	if (errno != 0 || *end != '\0')
		throw std::runtime_error("invalid port");
	//verifie si y a eu une erreur de conversion, par exemple dépassement ou si Toute la chaîne n’a pas été convertie.
	if (port <= 0 || port > 65535)
		throw std::runtime_error("port must be between 1 and 65535");
	//Un port TCP valide est entre 1 et 65535: les ports sous 1024 demandent souvent des droits administrateur. Pour les tests, utilise plutôt :
	return (static_cast<int>(port)); //onvertis le long en int après avoir vérifié qu’il est dans une plage valide
}

int	main(int argc, char **argv)
{
	try
	{
		if (argc != 3)
		{
			std::cerr << RED << "Usage: ./ircserv <port> <password>" << RESET << std::endl;
			return (1);
		}

		int	port = parsePort(argv[1]);
		std::string	password = argv[2];

		if (password.empty())
			throw std::runtime_error("password cannot be empty");

		ServerConfig	config(port, password);
		Server			server(config);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << B_RED << "Error: " << RED << e.what() << RESET << std::endl;
		return (1);
	}

	return (0);
}
/*
Server = gère tout le réseau
Client = stocke le fd + le buffer d’un utilisateur
_fds = liste surveillée par poll()
_clients = map fd -> Client
poll() = attend un événement
accept() = accepte un nouveau client
recv() = lit les données d’un client
_buffer = reconstruit les commandes coupées
popCommand() = sort une ligne complète du buffer
removeClient() = ferme et supprime un client
*/
/*
J’ai un socket serveur en non-bloquant. Je l’ajoute à un tableau de pollfd. 
Quand poll() détecte POLLIN sur le socket serveur, j’accepte un nouveau client 
avec accept(). Je mets ce client en non-bloquant, je l’ajoute aussi au tableau 
de poll(), et je crée un objet Client associé à son fd. Quand poll() détecte POLLIN 
sur un client, je lis avec recv(), j’ajoute les bytes reçus dans son buffer, 
puis je traite uniquement les lignes complètes terminées par \n. 
Si le client se déconnecte ou si une erreur arrive, je ferme son fd et je le retire 
de mes structures.
*/