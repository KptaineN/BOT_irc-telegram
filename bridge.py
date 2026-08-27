#!/usr/bin/env python3

import asyncio
import hmac
import os
import sys

from dotenv import load_dotenv

from telegram import Update
from telegram.ext import (
    ApplicationBuilder,
    CommandHandler,
    MessageHandler,
    ContextTypes,
    filters,
)

from irc_bot import Bot


# ============================================================
# CONFIGURATION
# ============================================================

load_dotenv()

TELEGRAM_TOKEN = os.getenv("TELEGRAM_BOT_TOKEN")
TELEGRAM_AUTH_PASSWORD = os.getenv("TELEGRAM_AUTH_PASSWORD")

IRC_HOST = os.getenv("IRC_HOST", "127.0.0.1")
IRC_PORT = int(os.getenv("IRC_PORT", "6667"))
IRC_PASSWORD = os.getenv("IRC_PASSWORD")
IRC_CHANNEL = os.getenv("IRC_CHANNEL", "#42")
IRC_NICK = os.getenv("IRC_NICK", "skeleton")


if not TELEGRAM_TOKEN:
    raise RuntimeError("TELEGRAM_BOT_TOKEN is not defined")

if not TELEGRAM_AUTH_PASSWORD:
    raise RuntimeError("TELEGRAM_AUTH_PASSWORD is not defined")


# ============================================================
# STATE
# ============================================================

telegram_app = None

# Telegram users authenticated with /auth
authorized_users = set()

# user_id -> chat_id
#
# Only users present here receive IRC messages.
subscriptions = {}


# ============================================================
# IRC BOT
# ============================================================

irc_bot = Bot(
    server=IRC_HOST,
    port=IRC_PORT,
    password=IRC_PASSWORD,
    channel=IRC_CHANNEL,
    nickname=IRC_NICK,
)


# ============================================================
# HELPERS
# ============================================================

def is_authorized(update: Update) -> bool:
    user = update.effective_user

    if user is None:
        return False

    return user.id in authorized_users


def irc_is_connected() -> bool:
    return (
        irc_bot.writer is not None
        and not irc_bot.writer.is_closing()
    )


async def require_auth(update: Update) -> bool:
    if is_authorized(update):
        return True

    if update.message:
        await update.message.reply_text(
            "Authentication required.\n"
            "Use /auth <password>"
        )

    return False


async def broadcast_to_telegram(
    text: str,
    exclude_chat_id=None
):
    """
    Send a message to every Telegram chat currently
    subscribed to the bridge.

    exclude_chat_id can be used to prevent sending the
    message back to the Telegram user who originally sent it.
    """

    global telegram_app

    if telegram_app is None:
        return

    # set() avoids sending the same message several times
    # if several users happen to share the same Telegram chat.
    chat_ids = set(subscriptions.values())

    if exclude_chat_id is not None:
        chat_ids.discard(exclude_chat_id)

    for chat_id in chat_ids:
        try:
            await telegram_app.bot.send_message(
                chat_id=chat_id,
                text=text,
            )

        except Exception as ex:
            print(
                f"[TELEGRAM] Failed to send "
                f"to chat {chat_id}: {ex}"
            )


# ============================================================
# TELEGRAM COMMANDS
# ============================================================

async def start_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if is_authorized(update):
        await update.message.reply_text(
            "Bridge already active for you.\n\n"
            f"IRC channel: {IRC_CHANNEL}\n"
            "Use /help to see commands."
        )
        return

    await update.message.reply_text(
        "Telegram <-> IRC Bridge\n\n"
        "Authentication is required.\n"
        "Use:\n"
        "/auth <password>"
    )


async def auth_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    user = update.effective_user

    if user is None:
        return

    if user.id in authorized_users:
        await update.message.reply_text(
            "Tu es déjà authentifié.\n"
            "Utilise /join pour rejoindre le bridge."
        )
        return

    if not context.args:
        await update.message.reply_text(
            "Utilisation : /auth <mot_de_passe>"
        )
        return

    provided_password = context.args[0]

    if not hmac.compare_digest(
        provided_password,
        TELEGRAM_AUTH_PASSWORD
    ):
        print(
            f"[AUTH] Failed authentication "
            f"for Telegram user {user.id}"
        )

        await update.message.reply_text(
            "Authentification refusée."
        )
        return

    authorized_users.add(user.id)

    username = (
        user.username
        or user.first_name
        or str(user.id)
    )

    print(
        f"[AUTH] {username} authenticated "
        f"(user_id={user.id})"
    )

    await update.message.reply_text(
        "Authentification réussie. ✅\n\n"
        "Utilise /join pour rejoindre le bridge IRC."
    )


async def join_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    user = update.effective_user
    chat = update.effective_chat

    if user is None or chat is None:
        return

    if user.id in subscriptions:
        await update.message.reply_text(
            f"Tu es déjà abonné à {IRC_CHANNEL}."
        )
        return

    subscriptions[user.id] = chat.id

    username = (
        user.username
        or user.first_name
        or str(user.id)
    )

    print(
        f"[BRIDGE] {username} joined "
        f"{IRC_CHANNEL} "
        f"(chat_id={chat.id})"
    )

    await update.message.reply_text(
        "Bridge rejoint. ✅\n"
        f"Channel IRC : {IRC_CHANNEL}\n"
        f"Utilisateurs Telegram connectés : "
        f"{len(subscriptions)}"
    )


async def leave_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    user = update.effective_user

    if user is None:
        return

    if user.id not in subscriptions:
        await update.message.reply_text(
            "Tu n'es actuellement abonné à aucun bridge."
        )
        return

    subscriptions.pop(user.id, None)

    print(
        f"[BRIDGE] Telegram user {user.id} "
        f"left {IRC_CHANNEL}"
    )

    await update.message.reply_text(
        f"Tu as quitté le bridge {IRC_CHANNEL}.\n"
        "Tu restes authentifié et peux utiliser "
        "/join pour revenir."
    )


async def logout_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    user = update.effective_user

    if user is None:
        return

    if user.id not in authorized_users:
        await update.message.reply_text(
            "Tu n'es pas authentifié."
        )
        return

    # Leaving the bridge automatically
    subscriptions.pop(user.id, None)

    # Removing authentication
    authorized_users.discard(user.id)

    print(
        f"[AUTH] Telegram user {user.id} logged out"
    )

    await update.message.reply_text(
        "Déconnexion effectuée.\n"
        "Tu dois refaire /auth pour accéder au bridge."
    )


async def ping_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    await update.message.reply_text("Pong!")


async def status_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    user = update.effective_user

    subscribed = (
        user is not None
        and user.id in subscriptions
    )

    irc_status = (
        "connected"
        if irc_is_connected()
        else "disconnected"
    )

    await update.message.reply_text(
        "Bridge status\n\n"
        f"Authentication: yes\n"
        f"Subscription: "
        f"{'yes' if subscribed else 'no'}\n"
        f"IRC: {irc_status}\n"
        f"Server: {IRC_HOST}:{IRC_PORT}\n"
        f"Channel: {IRC_CHANNEL}\n"
        f"IRC nick: {IRC_NICK}\n"
        f"Telegram users: {len(subscriptions)}"
    )


async def channel_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    await update.message.reply_text(
        f"Current IRC channel: {IRC_CHANNEL}"
    )


async def help_command(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    await update.message.reply_text(
        "Telegram <-> IRC Bridge\n\n"
        "/start - Accueil\n"
        "/auth <password> - S'authentifier\n"
        "/join - Rejoindre le bridge IRC\n"
        "/leave - Quitter le bridge IRC\n"
        "/logout - Se déconnecter complètement\n"
        "/status - État du bridge\n"
        "/channel - Channel IRC actuel\n"
        "/ping - Tester le bot\n"
        "/help - Afficher cette aide"
    )


# ============================================================
# TELEGRAM -> IRC
# ============================================================

async def telegram_message(
    update: Update,
    context: ContextTypes.DEFAULT_TYPE
):
    if not await require_auth(update):
        return

    if not update.message:
        return

    if not update.message.text:
        return

    user = update.effective_user
    chat = update.effective_chat

    if user is None or chat is None:
        return

    if user.id not in subscriptions:
        await update.message.reply_text(
            "Tu es authentifié mais tu n'as pas "
            "rejoint le bridge.\n"
            "Utilise /join."
        )
        return

    username = (
        user.username
        or user.first_name
        or str(user.id)
    )

    message = update.message.text

    print(
        f"[TELEGRAM -> IRC] "
        f"{username}: {message}"
    )

    # --------------------------------------------------------
    # Send to IRC
    # --------------------------------------------------------

    if not irc_is_connected():
        await update.message.reply_text(
            "IRC server is currently unavailable."
        )
        return

    try:
        irc_text = (
            f"[Telegram] {username}: {message}"
        )

        await irc_bot.sendmsg(
            IRC_CHANNEL,
            irc_text
        )

    except Exception as ex:
        print(
            f"[BRIDGE] Telegram -> IRC error: {ex}"
        )

        await update.message.reply_text(
            "Unable to send the message to IRC."
        )

        return

    # --------------------------------------------------------
    # Telegram -> other Telegram users
    #
    # IRC usually does not send the bot its own PRIVMSG back.
    # Therefore we fan-out the message ourselves.
    # --------------------------------------------------------

    await broadcast_to_telegram(
        f"[Telegram] {username}: {message}",
        exclude_chat_id=chat.id,
    )


# ============================================================
# IRC -> TELEGRAM
# ============================================================

async def irc_message(
    nick: str,
    target: str,
    message: str
):
    print(
        f"[IRC -> TELEGRAM] "
        f"{nick} -> {target}: {message}"
    )

    # Ignore messages from another IRC channel
    if target != IRC_CHANNEL:
        return

    # Important loop protection.
    #
    # Telegram -> skeleton -> IRC
    #
    # must NOT become:
    #
    # Telegram -> IRC -> Telegram -> IRC ...
    #
    if nick == IRC_NICK:
        return

    await broadcast_to_telegram(
        f"[IRC] {nick}: {message}"
    )


# Link the IRC client to the Telegram bridge
irc_bot.on_message = irc_message


# ============================================================
# STARTUP ANIMATION
# ============================================================

BOAT = (
    "                ,        ,\n"
    "           __.-'|'-.__.-'|'-.__\n"
    "         ='=====|========|====='="
)

WAVE_FRAMES = [
    "    ~_^~-^~~_~^-^~-~~^_~^~^~^",
    "    _~^_~^-^~~_~^~-^~_~~^~_^~",
    "    ^~_~~^_~^-^~_~~^-^~_^~~^_",
    "    ~^_~^-~~^_~^~-^~~_~^_^~^-",
]

CLEAR_SCREEN = "\033[2J\033[H"


async def startup_animation():
    loading_messages = [
        "Initializing bridge",
        "Preparing Telegram",
        "Preparing IRC",
        "Opening communication channel",
    ]

    frames = 20

    for i in range(frames):
        sys.stdout.write(CLEAR_SCREEN)

        print(BOAT)
        print(
            WAVE_FRAMES[
                i % len(WAVE_FRAMES)
            ]
        )
        print()

        message = loading_messages[
            min(
                i // 5,
                len(loading_messages) - 1
            )
        ]

        dots = "." * ((i % 3) + 1)

        print(
            f"        {message}{dots}"
        )

        sys.stdout.flush()

        await asyncio.sleep(0.10)

    sys.stdout.write(CLEAR_SCREEN)

    print(BOAT)
    print(WAVE_FRAMES[0])
    print()

    print(
        "        Telegram <-> IRC Bridge"
    )
    print(
        "        ========================"
    )
    print()

    sys.stdout.flush()


# ============================================================
# MAIN
# ============================================================

async def main():
    global telegram_app

    await startup_animation()

    print(
        f"IRC server  : "
        f"{IRC_HOST}:{IRC_PORT}"
    )
    print(
        f"IRC channel : {IRC_CHANNEL}"
    )
    print(
        f"IRC nick    : {IRC_NICK}"
    )
    print()

    # --------------------------------------------------------
    # Telegram
    # --------------------------------------------------------

    telegram_app = (
        ApplicationBuilder()
        .token(TELEGRAM_TOKEN)
        .build()
    )

    telegram_app.add_handler(
        CommandHandler(
            "start",
            start_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "auth",
            auth_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "join",
            join_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "leave",
            leave_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "logout",
            logout_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "status",
            status_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "channel",
            channel_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "ping",
            ping_command
        )
    )

    telegram_app.add_handler(
        CommandHandler(
            "help",
            help_command
        )
    )

    telegram_app.add_handler(
        MessageHandler(
            filters.TEXT
            & ~filters.COMMAND,
            telegram_message
        )
    )

    await telegram_app.initialize()
    await telegram_app.start()
    await telegram_app.updater.start_polling()

    print("[✓] Telegram connected")

    # --------------------------------------------------------
    # IRC
    # --------------------------------------------------------

    try:
        await irc_bot.connect()

    finally:
        print(
            "[BRIDGE] Shutting down..."
        )

        if telegram_app.updater.running:
            await telegram_app.updater.stop()

        if telegram_app.running:
            await telegram_app.stop()

        await telegram_app.shutdown()


# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    try:
        asyncio.run(main())

    except KeyboardInterrupt:
        print(
            "\n[BRIDGE] Stopped"
        )