<p align="center">
  <img src="https://media.giphy.com/media/v1.Y2lkPWVjZjA1ZTQ3OGI3OHFzNXl1bG1wMDVjNzU3ZHIzaXB0ZzdheWRlejdreHRwdWVwdyZlcD12MV9naWZzX3NlYXJjaCZjdD1n/15L4GgZxncHh6/giphy.gif"/>
</p>


# Telegram ↔ IRC Bridge

Passerelle bidirectionnelle entre Telegram et un serveur IRC (ft_irc), développée en Python. Le bridge connecte un bot Telegram à un client IRC (irc_bot.py) qui communique avec le serveur ft_irc (C++98) sur le port 6667, permettant à des utilisateurs Telegram autorisés d'échanger avec un channel IRC sans utiliser un client IRC.

## Fonctionnalités

- Bidirectionnel : Telegram → IRC et IRC → Telegram
- Authentification des utilisateurs Telegram (/auth)
- Abonnement/désabonnement au bridge (/join, /leave)
- Protection contre les boucles (le bridge ignore ses propres messages)
- Basé sur asyncio pour gérer simultanément la connexion IRC et les événements Telegram
- Séparation claire entre configuration, logique IRC et logique Telegram

## Architecture (schéma simplifié)

Telegram
   │
   ▼
Telegram Bot API
   │
   ▼
bridge.py (auth, routing, subscriptions)
   │
   ▼
irc_bot.py (client IRC asyncio)
   │
   ▼
ft_irc (serveur C++98) :6667
   │
   ▼
#42 (channel IRC)

## Prérequis

- Python 3.8+ (ou version compatible avec python-telegram-bot et asyncio)
- python-telegram-bot
- python-dotenv
- Un serveur ft_irc (exécutable `ircserv`) écoutant sur le port configuré (par défaut 6667)

## Installation

1. Cloner le repository :

   git clone <repository>
   cd telegram-irc-bridge

2. Créer et activer un environnement virtuel :

   python3 -m venv .venv
   source .venv/bin/activate

3. Installer les dépendances :

   .venv/bin/pip install -r requirements.txt

   ou directement :

   .venv/bin/pip install python-telegram-bot python-dotenv

4. Copier l'exemple de configuration :

   cp .env.example .env

   et renseigner les valeurs.

## Configuration (.env)

Les paramètres et secrets sont stockés dans `.env`. Exemple :

TELEGRAM_BOT_TOKEN=YOUR_TELEGRAM_TOKEN
TELEGRAM_AUTH_PASSWORD=YOUR_BRIDGE_PASSWORD

IRC_HOST=127.0.0.1
IRC_PORT=6667
IRC_PASSWORD=
IRC_CHANNEL=#42
IRC_NICK=skeleton

Important : `.env` contient des secrets (token Telegram, mots de passe) — ne pas committer ce fichier. Ajouter `.env` et `.venv/` dans `.gitignore`. Conserver `.env.example` dans le dépôt pour guider la configuration.

## Lancement

1. Démarrer le serveur IRC (ft_irc) :

   ./ircserv 6667 42

2. Lancer le bridge :

   .venv/bin/python bridge.py

   ou rendre le script `start` exécutable et l'utiliser :

   chmod +x start
   ./start

Exemple de script `start` :

#!/bin/bash
DIR="$(cd "$(dirname "${0}")" && pwd)"
exec "$DIR/.venv/bin/python" "$DIR/bridge.py"

## Commandes Telegram

- /start — accueil
- /auth <password> — authentifie l'utilisateur auprès du bridge
- /join — abonne le chat au bridge (reçoit les messages IRC)
- /leave — quitte le bridge (mais reste authentifié)
- /logout — supprime l'authentification
- /status — affiche l'état du bridge
- /channel — affiche le channel IRC utilisé
- /ping — vérifie que le bot répond
- /help — affiche l'aide

Remarque : /leave conserve l'authentification (permet de /join ensuite sans ré-auth), tandis que /logout supprime l'authentification et nécessite un nouveau /auth.

## Comportement des messages

- Telegram → IRC :
  - Un message Telegram est envoyé sur le channel IRC sous la forme :
    `PRIVMSG #42 :[Telegram] <username>: <message>`
  - Le bot utilise le nickname configuré (ex: `skeleton`) pour poster sur IRC.

- IRC → Telegram :
  - Un message IRC `:nick!user@host PRIVMSG #42 :message` est relayé vers tous les chats Telegram abonnés sous la forme :
    `[IRC] <nick>: <message>`

- Protection contre les boucles :
  - Les messages émis par le bot sur IRC sont ignorés en réception si `nick == IRC_NICK`.

## Sécurité

- Ne jamais committer `TELEGRAM_BOT_TOKEN`, `TELEGRAM_AUTH_PASSWORD` ou `IRC_PASSWORD` dans le dépôt.
- Garder `.env` hors du contrôle de version.
- Le token Telegram est particulièrement sensible — quiconque le possède peut agir au nom du bot.
- Le bridge utilise un mot de passe distinct (`TELEGRAM_AUTH_PASSWORD`) pour contrôler qui peut publier vers IRC via le bot Telegram.

## Exemples

Exemple d'échange (IRC → Telegram) :

14:11 <@Marwin> Salut Telegram !
Bridge log:
[IRC MESSAGE] Marwin -> #42: Salut Telegram!
Telegram:
[IRC] Marwin: Salut Telegram!

Exemple (Telegram → IRC) :

Telegram (Nkief57): Salut IRC !
Bridge log:
[TELEGRAM -> IRC] Nkief57: Salut IRC!
IRC:
<skeleton> [Telegram] Nkief57: Salut IRC!

## Limitations et évolutions possibles

Limitations actuelles :
- Les listes `authorized_users` et `subscriptions` sont conservées en mémoire (pas de persistance). Après redémarrage, ré-authentification requise.

Évolutions envisagées :
- Persistance des utilisateurs autorisés
- Support multi-channel (/channels, /join #name, /leave #name)
- Commandes /users, /channels
- Gestion plus fine des logs et des erreurs
- Limitation du flood vers Telegram
- Rôles (admin/utilisateur)
- Messages privés Telegram ↔ IRC
- Notifications JOIN/PART/QUIT
- Lancement en tant que service systemd ou Dockerisation

## Structure du projet

- bridge.py — point d'entrée : orchestration, authentification, routage
- irc_bot.py — client IRC réutilisable (connexion, parsing, envoi de PRIVMSG, PING/PONG)
- requirements.txt — dépendances Python
- .env.example — modèle de configuration (sans secrets)
- start — script de lancement
- README.md — ce fichier

## Tests et usage comme outil de validation

Le bridge a été utile pour tester le serveur ft_irc : PASS/NICK/USER, RPL_WELCOME (001), JOIN, RPL_NAMREPLY (353), PRIVMSG, PING/PONG, etc. Utiliser un bot externe est un bon moyen de vérifier la compatibilité du serveur avec des clients réels.

## Crédits

La partie client IRC est initialement inspirée du projet "Skeleton" d'AcidVegas, puis adaptée pour l'intégration au bridge Telegram ↔ ft_irc.

---

Pour toute question, demande d'ajout de fonctionnalité ou rapport de bug, ouvrir une issue dans le dépôt.
