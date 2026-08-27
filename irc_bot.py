#!/usr/bin/env python3

import asyncio
import logging
import time


class Bot:
    def __init__(
        self,
        server,
        port,
        password,
        channel,
        nickname="skeleton"
    ):
        self.server = server
        self.port = port
        self.password = password
        self.channel = channel

        self.nickname = nickname
        self.username = "skelly"
        self.realname = "Telegram IRC Bridge"

        self.reader = None
        self.writer = None

        self.last = time.time()

        # Callback utilisé plus tard pour IRC -> Telegram
        self.on_message = None

    async def raw(self, data: str):
        """
        Envoie une commande IRC brute.
        """

        if self.writer is None:
            raise RuntimeError("IRC bot is not connected")

        # Never log the IRC password
        if data.upper().startswith("PASS "):
            print("[IRC ->] PASS ****")
        else:
            print(f"[IRC ->] {data}")

        self.writer.write(
            data[:510].encode("utf-8") + b"\r\n"
        )

        await self.writer.drain()

    async def sendmsg(self, target: str, msg: str):
        """
        Envoie un PRIVMSG vers un channel ou un utilisateur.
        """

        await self.raw(
            f"PRIVMSG {target} :{msg}"
        )

    async def connect(self):
        """
        Connexion au serveur IRC.

        Le bot se reconnecte automatiquement
        si la connexion est perdue.
        """

        while True:
            try:
                print(
                    f"[IRC] Connecting to "
                    f"{self.server}:{self.port}"
                )

                self.reader, self.writer = (
                    await asyncio.wait_for(
                        asyncio.open_connection(
                            host=self.server,
                            port=self.port,
                            family=2,  # IPv4
                        ),
                        timeout=15,
                    )
                )

                print("[IRC] Connected")

                # Registration IRC
                if self.password:
                    await self.raw(
                        f"PASS {self.password}"
                    )

                await self.raw(
                    f"NICK {self.nickname}"
                )

                await self.raw(
                    f"USER {self.username} "
                    f"0 * :{self.realname}"
                )

                # Lecture des messages du serveur
                while not self.reader.at_eof():

                    data = await asyncio.wait_for(
                        self.reader.readuntil(b"\r\n"),
                        timeout=300,
                    )

                    message = (
                        data.decode(
                            "utf-8",
                            errors="replace"
                        )
                        .rstrip("\r\n")
                    )

                    await self.handle(message)

            except asyncio.TimeoutError:
                print("[IRC] Connection timeout")

            except ConnectionRefusedError:
                print(
                    "[IRC] Connection refused. "
                    "Is ircserv running?"
                )

            except (
                ConnectionResetError,
                BrokenPipeError,
                asyncio.IncompleteReadError,
            ):
                print("[IRC] Connection lost")

            except Exception as ex:
                logging.exception(
                    f"[IRC] Error: {ex}"
                )

            finally:
                if self.writer is not None:
                    try:
                        self.writer.close()
                        await self.writer.wait_closed()
                    except Exception:
                        pass

                self.reader = None
                self.writer = None

                print(
                    "[IRC] Reconnecting in 5 seconds..."
                )

                await asyncio.sleep(5)

    async def handle(self, data: str):
        """
        Traite une ligne reçue du serveur IRC.
        """

        print(f"[IRC <-] {data}")

        parts = data.split()

        if not parts:
            return

        # PING -> PONG
        if parts[0] == "PING":
            if len(parts) > 1:
                await self.raw(
                    f"PONG {parts[1]}"
                )
            return

        if len(parts) < 2:
            return

        command = parts[1]

        # 001 = registration réussie
        if command == "001":
            print(
                f"[IRC] Registered as "
                f"{self.nickname}"
            )

            print(
                f"[IRC] Joining "
                f"{self.channel}"
            )

            await self.raw(
                f"JOIN {self.channel}"
            )

        # Nick déjà utilisé
        elif command == "433":
            self.nickname += "_"

            print(
                f"[IRC] Nick already used. "
                f"Trying {self.nickname}"
            )

            await self.raw(
                f"NICK {self.nickname}"
            )

        elif command == "PRIVMSG":
            await self.event_privmsg(data)

        elif command == "INVITE":
            if len(parts) >= 4:
                target = parts[2]
                channel = parts[3].lstrip(":")

                if target == self.nickname:
                    await self.raw(
                        f"JOIN {channel}"
                    )

        elif command == "KICK":
            if len(parts) >= 4:
                channel = parts[2]
                kicked = parts[3]

                if kicked == self.nickname:
                    print(
                        f"[IRC] Kicked from "
                        f"{channel}"
                    )

                    await asyncio.sleep(3)

                    await self.raw(
                        f"JOIN {channel}"
                    )

    async def event_privmsg(self, data: str):
        """
        Traite un PRIVMSG reçu depuis IRC.

        Exemple :
        :Marwin!kiefer@localhost
        PRIVMSG #42 :Salut
        """

        parts = data.split()

        if len(parts) < 4:
            return

        prefix = parts[0]

        nick = (
            prefix
            .split("!")[0]
            .lstrip(":")
        )

        target = parts[2]

        message = " ".join(parts[3:])

        if message.startswith(":"):
            message = message[1:]

        print(
            f"[IRC MESSAGE] "
            f"{nick} -> {target}: {message}"
        )

        # Plus tard :
        # IRC -> Telegram
        if self.on_message is not None:
            await self.on_message(
                nick,
                target,
                message
            )

        # On conserve le test !ping
        if (
            target.startswith("#")
            and message == "!ping"
        ):
            await self.sendmsg(
                target,
                "Pong!"
            )