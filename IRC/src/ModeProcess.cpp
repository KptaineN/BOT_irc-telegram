#include "../includes/ModeProcess.hpp"
#include "../includes/Icommande.hpp"
#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"

#include <cctype>
#include <limits>
#include <map>
#include <sstream>

/*
 * Parcours d'une commande MODE :
 *
 * CommandDispatcher -> ModeCommand::execute() -> ModeProcess::execute()
 *
 * Une commande sans chaîne de modes consulte l'état du canal.
 * Une commande avec une chaîne de modes modifie le canal, puis diffuse
 * uniquement les changements réellement appliqués à ses membres.
 */

// Produit une copie en minuscules pour rechercher un nickname sans tenir
// compte de la casse lorsque la recherche exacte du serveur échoue.
static std::string modeLower(const std::string& value)
{
	std::string result(value);
	std::size_t i;

	i = 0;
	while (i < result.size())
	{
		result[i] = static_cast<char>(std::tolower(
			static_cast<unsigned char>(result[i])));
		++i;
	}
	return (result);
}

ModeProcess::ModeProcess(Server& server, Client& client,
	const IrcMessage& msg, Channel& channel)
	: _server(server), _client(client), _msg(msg), _channel(channel),
	  _adding(true), _argumentIndex(2)
{
	/*
	 * params[0] : nom du canal
	 * params[1] : chaîne de modes, par exemple "+ko"
	 * params[2] et suivants : arguments de k, o ou l
	 */
}

ModeProcess::~ModeProcess()
{
}

void ModeProcess::processQuery()
{
	std::string modes("+");
	std::vector<std::string> arguments;
	std::ostringstream limit;
	std::string reply;
	std::size_t i;

	// Reconstruit les modes actifs dans un ordre stable pour la réponse 324.
	if (_channel.hasMode('i'))
		modes += 'i';
	if (_channel.hasMode('t'))
		modes += 't';
	// La clé n'est révélée qu'à un membre du canal.
	if (_channel.hasMode('k') && _channel.hasClient(_client.getFd()))
	{
		modes += 'k';
		arguments.push_back(_channel.getKey());
	}
	if (_channel.hasMode('l'))
	{
		modes += 'l';
		limit << _channel.getUserLimit();
		arguments.push_back(limit.str());
	}
	// Les arguments doivent suivre la chaîne de modes dans le même ordre.
	reply = _channel.getName() + " " + modes;
	i = 0;
	while (i < arguments.size())
	{
		reply += " " + arguments[i];
		++i;
	}
	_server.reply(_client, "324", reply);
}

void ModeProcess::appendModeChange(bool adding, char mode)
{
	char sign;
	char lastSign;
	std::size_t plus;
	std::size_t minus;

	/*
	 * Construit une chaîne compacte destinée au broadcast.
	 * Exemple : ajout de i et t, puis retrait de k => "+it-k".
	 */
	sign = adding ? '+' : '-';
	plus = _changes.rfind('+');
	minus = _changes.rfind('-');
	lastSign = '\0';
	if (plus == std::string::npos && minus != std::string::npos)
		lastSign = '-';
	else if (minus == std::string::npos && plus != std::string::npos)
		lastSign = '+';
	else if (plus != std::string::npos && minus != std::string::npos)
		lastSign = plus > minus ? '+' : '-';
	if (lastSign != sign)
		_changes += sign;
	_changes += mode;
}

	// i et t n'ont pas d'argument : il suffit de modifier leur présence.
	// Une demande qui ne change pas l'état actuel n'est pas rediffusée.
void ModeProcess::applySimpleMode(char mode)
{
	if (_channel.hasMode(mode) == _adding)
		return ;
	if (_adding)
		_channel.addMode(mode);
	else
		_channel.removeMode(mode);
	appendModeChange(_adding, mode);
}

bool ModeProcess::handleKeyMode() //cas k
{
	std::string key;

	// -k retire le mode et efface aussi la clé conservée par Channel.
	if (!_adding)
	{
		if (_channel.hasMode('k'))
		{
			_channel.removeMode('k');
			_channel.setKey("");
			appendModeChange(false, 'k');
		}
		return (true);
	}
	// +k exige la clé placée dans le prochain argument de la commande.
	if (_argumentIndex >= _msg.params.size()
		|| _msg.params[_argumentIndex].empty())
	{
		_server.reply(_client, "461", "MODE :Not enough parameters");
		return (false);
	}
	key = _msg.params[_argumentIndex++];
	// Enregistre et prépare la diffusion seulement si la clé change.
	if (!_channel.hasMode('k') || _channel.getKey() != key)
	{
		_channel.addMode('k');
		_channel.setKey(key);
		appendModeChange(true, 'k');
		_changeArguments.push_back(key);
	}
	return (true);
}

Client* ModeProcess::findClient(const std::string& nickname)
{
	Client* client;
	std::string wanted;
	std::map<int, Client>& clients = _server.getState().getClients();
	std::map<int, Client>::iterator it;

	// Privilégie la recherche exacte, moins coûteuse et non ambiguë.
	client = _server.getState().getClientByNickname(nickname);
	if (client != NULL)
		return (client);
	// Les nicknames IRC sont comparés sans tenir compte de la casse.
	wanted = modeLower(nickname);
	it = clients.begin();
	while (it != clients.end())
	{
		if (modeLower(it->second.getProfile().getNickname()) == wanted)
			return (&it->second);
		++it;
	}
	return (NULL);
}

bool ModeProcess::handleOperatorMode() //cas o
{
	std::string nickname;
	Client* target;

	// +o et -o exigent tous les deux le nickname de la cible.
	if (_argumentIndex >= _msg.params.size()
		|| _msg.params[_argumentIndex].empty())
	{
		_server.reply(_client, "461", "MODE :Not enough parameters");
		return (false);
	}
	nickname = _msg.params[_argumentIndex++];
	target = findClient(nickname);
	// 401 : le client n'existe pas sur le serveur.
	if (target == NULL)
	{
		_server.reply(_client, "401",
			nickname + " :No such nick/channel");
		return (true);
	}
	// 441 : le client existe, mais ne fait pas partie de ce canal.
	if (!_channel.hasClient(target->getFd()))
	{
		_server.reply(_client, "441",
			target->getProfile().getNickname() + " " + _channel.getName()
			+ " :They aren't on that channel");
		return (true);
	}
	// Ne diffuse rien si la cible possède déjà l'état demandé.
	if (_channel.isOperator(target->getFd()) != _adding)
	{
		if (_adding)
			_channel.addOperator(target);
		else
			_channel.removeOperator(target);
		appendModeChange(_adding, 'o');
		_changeArguments.push_back(target->getProfile().getNickname());
	}
	return (true);
}

bool ModeProcess::parseUserLimit(const std::string& value,
	std::size_t& limit) const
{
	std::size_t i;
	std::size_t parsed;
	unsigned int digit;
	const std::size_t maximum = std::numeric_limits<std::size_t>::max();

	// La limite doit être un entier strictement positif écrit en décimal.
	if (value.empty())
		return (false);
	i = 0;
	parsed = 0;
	while (i < value.size())
	{
		if (!std::isdigit(static_cast<unsigned char>(value[i])))
			return (false);
		digit = static_cast<unsigned int>(value[i] - '0');
		// Vérifie le débordement avant d'effectuer parsed * 10 + digit.
		if (parsed > (maximum - digit) / 10)
			return (false);
		parsed = parsed * 10 + digit;
		++i;
	}
	if (parsed == 0)
		return (false);
	limit = parsed;
	return (true);
}

bool ModeProcess::handleLimitMode() //cas l
{
	std::string value;
	std::size_t limit;

	// -l n'attend aucun argument et remet la limite interne à zéro.
	if (!_adding)
	{
		if (_channel.hasMode('l'))
		{
			_channel.removeMode('l');
			_channel.setUserLimit(0);
			appendModeChange(false, 'l');
		}
		return (true);
	}
	// +l consomme le prochain argument comme nombre maximal de membres.
	if (_argumentIndex >= _msg.params.size())
	{
		_server.reply(_client, "461", "MODE :Not enough parameters");
		return (false);
	}
	value = _msg.params[_argumentIndex++];
	if (!parseUserLimit(value, limit))
	{
		_server.reply(_client, "461",
			_channel.getName() + " :Invalid user limit");
		return (true);
	}
	// Une limite identique à la valeur courante ne produit aucun broadcast.
	if (!_channel.hasMode('l') || _channel.getUserLimit() != limit)
	{
		_channel.addMode('l');
		_channel.setUserLimit(limit);
		appendModeChange(true, 'l');
		_changeArguments.push_back(value);
	}
	return (true);
}

void ModeProcess::broadcastChanges()
{
	std::string message;
	std::size_t i;

	// Ne produit pas de message MODE lorsqu'aucun état n'a changé.
	if (_changes.empty())
		return ;
	// Préfixe IRC de l'opérateur à l'origine de la modification.
	message = ":" + _client.getProfile().getNickname()
		+ "!" + _client.getProfile().getUsername()
		+ "@localhost MODE " + _channel.getName() + " " + _changes;
	// Ajoute les clés, nicknames et limites associés aux modes diffusés.
	i = 0;
	while (i < _changeArguments.size())
	{
		message += " " + _changeArguments[i];
		++i;
	}
	// NULL demande à Channel d'envoyer aussi le message à son auteur.
	_channel.broadcast(_server, message, NULL);
}

void ModeProcess::processChanges()
{
	const std::string& modes = _msg.params[1];
	std::size_t i;
	bool keepProcessing;
	char mode;

	// Lit la chaîne de gauche à droite en conservant le dernier signe vu.
	// Exemple : "+it-k+l" alterne entre ajout et retrait.
	i = 0;
	keepProcessing = true;
	while (i < modes.size() && keepProcessing)
	{
		mode = modes[i++];
		if (mode == '+' || mode == '-')
		{
			_adding = mode == '+';
			continue ;
		}
		switch (mode)
		{
			case 'i':
				// fall through
			case 't':
				applySimpleMode(mode);
				break ;
			case 'k':
				keepProcessing = handleKeyMode();
				break ;
			case 'o':
				keepProcessing = handleOperatorMode();
				break ;
			case 'l':
				keepProcessing = handleLimitMode();
				break ;
			default:
				_server.reply(_client, "472", std::string(1, mode)
					+ " :is unknown mode char to me for "
					+ _channel.getName());
		}
	}
	broadcastChanges(); // Regroupe tous les changements valides en un appel
}

void ModeProcess::execute()
{
	// MODE #canal : consultation autorisée sans privilège opérateur.
	if (_msg.params.size() == 1)
	{
		processQuery();
		return ;
	}
	// Toute modification des modes exige les droits opérateur du canal.
	if (!_channel.isOperator(_client.getFd()))
	{
		_server.reply(_client, "482", _channel.getName()
			+ " :You're not channel operator");
		return ;
	}
	processChanges();
}

void ModeCommand::execute(Server& server, Client& client,
	const IrcMessage& msg)
{
	Channel* channel;

	// entrypoint : valide la cmd avant de créer le Process.
	if (!client.getProfile().isRegistered())
	{
		server.reply(client, "451", ":You have not registered");
		return ;
	}
	if (msg.params.empty())
	{
		server.reply(client, "461", "MODE :Not enough parameters");
		return ;
	}
	if (msg.params[0].empty()
		|| (msg.params[0][0] != '#' && msg.params[0][0] != '&'))
	{
		server.reply(client, "403", msg.params[0] + " :No such channel");
		return ;
	}
	// MODE travaille toujours sur un canal existant!
	channel = server.getChannel(msg.params[0]);
	if (channel == NULL)
	{
		server.reply(client, "403", msg.params[0] + " :No such channel");
		return ;
	}
	// L'objet temporaire garde tout l'état nécessaire au parsing de ce MODE.
	ModeProcess(server, client, msg, *channel).execute();
}
