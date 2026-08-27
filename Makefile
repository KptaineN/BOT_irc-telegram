PYTHON          := .venv/bin/python
PIP             := .venv/bin/pip
VENV            := .venv
BRIDGE          := bridge.py

IRC_DIR         := IRC
IRC_BIN         := $(IRC_DIR)/ircserv
IRC_PORT        := 6667
IRC_PASSWORD    := 42
IRC_PID         := .ircserv.pid

COMPOSE         := docker compose

.PHONY: all help setup check irc build-irc start-irc stop-irc \
        bridge run local docker build up down restart logs \
        clean fclean re

# ============================================================
# DEFAULT
# ============================================================

all: run


# ============================================================
# HELP
# ============================================================

help:
	@echo ""
	@echo "Telegram <-> IRC Bridge"
	@echo "========================"
	@echo ""
	@echo "Main commands:"
	@echo "  make           Build/start IRC server and launch bridge"
	@echo "  make run       Same as make"
	@echo "  make help      Show this help"
	@echo ""
	@echo "Local environment:"
	@echo "  make setup     Create .venv and install dependencies"
	@echo "  make bridge    Launch only the Python bridge"
	@echo ""
	@echo "IRC server:"
	@echo "  make irc       Build and start IRC server"
	@echo "  make build-irc Build IRC server"
	@echo "  make start-irc Start IRC server"
	@echo "  make stop-irc  Stop IRC server"
	@echo ""
	@echo "Docker:"
	@echo "  make build     Build Docker image"
	@echo "  make up        Start Docker bridge"
	@echo "  make docker    Build and start Docker bridge"
	@echo "  make logs      Follow Docker logs"
	@echo "  make down      Stop Docker containers"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean"
	@echo "  make fclean"
	@echo "  make re"
	@echo ""


# ============================================================
# CHECK
# ============================================================

check:
	@test -f "$(BRIDGE)" || \
		(echo "[ERROR] $(BRIDGE) not found"; exit 1)

	@test -f ".env" || \
		(echo "[ERROR] .env not found"; exit 1)

	@test -f "requirements.txt" || \
		(echo "[ERROR] requirements.txt not found"; exit 1)


# ============================================================
# PYTHON ENVIRONMENT
# ============================================================

setup: check
	@if [ ! -d "$(VENV)" ]; then \
		echo "[SETUP] Creating Python virtual environment..."; \
		python3 -m venv $(VENV); \
	else \
		echo "[SETUP] Virtual environment already exists."; \
	fi

	@echo "[SETUP] Installing Python dependencies..."
	@$(PIP) install -r requirements.txt

	@echo "[OK] Python environment ready."


# ============================================================
# IRC SERVER
# ============================================================

build-irc:
	@test -d "$(IRC_DIR)" || \
		(echo "[ERROR] IRC directory not found"; exit 1)

	@echo "[IRC] Building server..."
	@$(MAKE) -C $(IRC_DIR)

	@test -x "$(IRC_BIN)" || \
		(echo "[ERROR] IRC server executable not found"; exit 1)

	@echo "[OK] IRC server built."


start-irc: build-irc
	@if [ -f "$(IRC_PID)" ] && kill -0 $$(cat $(IRC_PID)) 2>/dev/null; then \
		echo "[IRC] Server already running (PID $$(cat $(IRC_PID)))."; \
	else \
		rm -f $(IRC_PID); \
		echo "[IRC] Starting server on port $(IRC_PORT)..."; \
		cd $(IRC_DIR) && \
		./ircserv $(IRC_PORT) $(IRC_PASSWORD) > ../ircserv.log 2>&1 & \
		echo $$! > $(IRC_PID); \
		sleep 1; \
		if kill -0 $$(cat $(IRC_PID)) 2>/dev/null; then \
			echo "[OK] IRC server running (PID $$(cat $(IRC_PID)))."; \
		else \
			echo "[ERROR] IRC server failed to start."; \
			rm -f $(IRC_PID); \
			exit 1; \
		fi \
	fi


stop-irc:
	@if [ -f "$(IRC_PID)" ]; then \
		PID=$$(cat $(IRC_PID)); \
		if kill -0 $$PID 2>/dev/null; then \
			echo "[IRC] Stopping server (PID $$PID)..."; \
			kill $$PID; \
			echo "[OK] IRC server stopped."; \
		else \
			echo "[IRC] PID file exists but process is not running."; \
		fi; \
		rm -f $(IRC_PID); \
	else \
		echo "[IRC] Server is not running."; \
	fi


irc: start-irc


# ============================================================
# LOCAL BRIDGE
# ============================================================

bridge: check
	@if [ ! -x "$(PYTHON)" ]; then \
		echo "[ERROR] Python virtual environment not found."; \
		echo "Run: make setup"; \
		exit 1; \
	fi

	@echo ""
	@echo "[BRIDGE] Starting in..."
	@sleep 1
	@echo "3"
	@sleep 1
	@echo "2"
	@sleep 1
	@echo "1"
	@sleep 1
	@echo ""

	@$(PYTHON) $(BRIDGE)


run: check
	@if [ ! -x "$(PYTHON)" ]; then \
		$(MAKE) setup; \
	fi

	@$(MAKE) start-irc
	@$(MAKE) bridge


local: run


# ============================================================
# DOCKER
# ============================================================

build: check
	@echo "[DOCKER] Building bridge..."
	@$(COMPOSE) build


up: check
	@echo "[DOCKER] Starting bridge..."
	@$(COMPOSE) up -d
	@echo "[OK] Docker bridge started."


docker: build up


logs:
	@$(COMPOSE) logs -f


down:
	@echo "[DOCKER] Stopping containers..."
	@$(COMPOSE) down


restart: stop-irc
	@$(COMPOSE) down 2>/dev/null || true
	@$(MAKE) run


# ============================================================
# CLEANUP
# ============================================================

clean:
	@echo "[CLEAN] Stopping services..."
	@$(MAKE) stop-irc
	@$(COMPOSE) down 2>/dev/null || true
	@rm -rf __pycache__
	@rm -f ircserv.log


fclean: clean
	@echo "[CLEAN] Removing Python environment..."
	@rm -rf $(VENV)

	@echo "[CLEAN] Cleaning IRC build..."
	@if [ -d "$(IRC_DIR)" ]; then \
		$(MAKE) -C $(IRC_DIR) fclean; \
	fi

	@$(COMPOSE) down --rmi local --volumes --remove-orphans 2>/dev/null || true

	@echo "[OK] Full clean complete."


re: fclean
	@$(MAKE) run
