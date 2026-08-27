# ============================================================
# Telegram <-> IRC Bridge
# ============================================================

PYTHON          := .venv/bin/python
PIP             := .venv/bin/pip
VENV            := .venv

BRIDGE          := bridge.py
BRIDGE_ABS      := $(abspath $(BRIDGE))

IRC_DIR         := IRC
IRC_BIN         := $(IRC_DIR)/ircserv
IRC_PORT        := 6667
IRC_PASSWORD    := 42

IRC_PID         := .ircserv.pid
IRC_LOG         := ircserv.log

COMPOSE         := docker compose
DOCKER_SERVICE  := bridge

# Small delay before starting the bridge
START_DELAY     := 3


.PHONY: all help check setup \
        build-irc start-irc stop-irc irc \
        check-local-bridge check-docker-bridge \
        bridge run local \
        docker-build docker-up docker-down docker logs \
        status stop restart \
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
	@echo "Main:"
	@echo "  make             Start IRC server if needed + local bridge"
	@echo "  make run         Same as make"
	@echo "  make status      Show IRC / local bridge / Docker state"
	@echo "  make stop        Stop services started by Make/Docker"
	@echo "  make restart     Restart local stack"
	@echo ""
	@echo "Manual mode:"
	@echo "  ./IRC/ircserv $(IRC_PORT) <password>"
	@echo "  ./start"
	@echo ""
	@echo "IRC:"
	@echo "  make build-irc   Compile IRC server"
	@echo "  make start-irc   Start IRC server if necessary"
	@echo "  make stop-irc    Stop IRC server started by Make"
	@echo ""
	@echo "Local bridge:"
	@echo "  make setup       Create .venv + install dependencies"
	@echo "  make bridge      Start bridge only"
	@echo ""
	@echo "Docker:"
	@echo "  make docker      Build + start Docker bridge"
	@echo "  make docker-up   Start Docker bridge"
	@echo "  make docker-down Stop Docker bridge"
	@echo "  make logs        Follow Docker logs"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean       Stop services + remove temporary files"
	@echo "  make fclean      Also remove venv + IRC build + Docker image"
	@echo "  make re          Full clean + local restart"
	@echo ""


# ============================================================
# BASIC CHECKS
# ============================================================

check:
	@test -f "$(BRIDGE)" || \
		(echo "[ERROR] $(BRIDGE) not found"; exit 1)

	@test -f ".env" || \
		(echo "[ERROR] .env not found"; exit 1)

	@test -f "requirements.txt" || \
		(echo "[ERROR] requirements.txt not found"; exit 1)

	@test -d "$(IRC_DIR)" || \
		(echo "[ERROR] $(IRC_DIR)/ directory not found"; exit 1)

	@test -f "$(IRC_DIR)/Makefile" || \
		(echo "[ERROR] $(IRC_DIR)/Makefile not found"; exit 1)


# ============================================================
# PYTHON ENVIRONMENT
# ============================================================

setup: check
	@if [ ! -x "$(PYTHON)" ]; then \
		echo "[SETUP] Creating Python virtual environment..."; \
		python3 -m venv "$(VENV)"; \
	else \
		echo "[SETUP] Python virtual environment already exists."; \
	fi

	@echo "[SETUP] Installing dependencies..."
	@$(PIP) install -r requirements.txt

	@echo "[OK] Python environment ready."


# ============================================================
# IRC SERVER
# ============================================================

build-irc: check
	@echo "[IRC] Building server..."
	@$(MAKE) -C "$(IRC_DIR)"

	@test -x "$(IRC_BIN)" || \
		(echo "[ERROR] $(IRC_BIN) was not generated"; exit 1)

	@echo "[OK] IRC server built."


start-irc: build-irc
	@set -e; \
	\
	if [ -f "$(IRC_PID)" ]; then \
		PID=$$(cat "$(IRC_PID)" 2>/dev/null || true); \
		if [ -n "$$PID" ] && kill -0 "$$PID" 2>/dev/null; then \
			echo "[IRC] Server already running (PID $$PID)."; \
			exit 0; \
		else \
			echo "[IRC] Removing stale PID file."; \
			rm -f "$(IRC_PID)"; \
		fi; \
	fi; \
	\
	if command -v ss >/dev/null 2>&1 && \
	   ss -ltn | awk '{print $$4}' | grep -Eq '(^|:)$(IRC_PORT)$$'; then \
		echo "[IRC] Port $(IRC_PORT) is already listening."; \
		echo "[IRC] Assuming an IRC server was started manually."; \
		echo "[IRC] Make will NOT start or manage another one."; \
		exit 0; \
	fi; \
	\
	echo "[IRC] Starting server on port $(IRC_PORT)..."; \
	cd "$(IRC_DIR)" && \
	./ircserv "$(IRC_PORT)" "$(IRC_PASSWORD)" > "../$(IRC_LOG)" 2>&1 & \
	PID=$$!; \
	echo "$$PID" > "$(IRC_PID)"; \
	sleep 1; \
	\
	if kill -0 "$$PID" 2>/dev/null; then \
		echo "[OK] IRC server running (PID $$PID)."; \
	else \
		echo "[ERROR] IRC server failed to start."; \
		echo "[ERROR] Check $(IRC_LOG)"; \
		rm -f "$(IRC_PID)"; \
		exit 1; \
	fi


stop-irc:
	@set -e; \
	if [ ! -f "$(IRC_PID)" ]; then \
		echo "[IRC] No Make-managed IRC server to stop."; \
		exit 0; \
	fi; \
	\
	PID=$$(cat "$(IRC_PID)" 2>/dev/null || true); \
	\
	if [ -n "$$PID" ] && kill -0 "$$PID" 2>/dev/null; then \
		echo "[IRC] Stopping Make-managed server (PID $$PID)..."; \
		kill "$$PID"; \
		\
		for i in 1 2 3 4 5; do \
			if ! kill -0 "$$PID" 2>/dev/null; then \
				break; \
			fi; \
			sleep 1; \
		done; \
		\
		if kill -0 "$$PID" 2>/dev/null; then \
			echo "[WARN] IRC server did not stop gracefully."; \
			echo "[WARN] PID $$PID was left running."; \
		else \
			echo "[OK] IRC server stopped."; \
		fi; \
	else \
		echo "[IRC] PID file was stale."; \
	fi; \
	\
	rm -f "$(IRC_PID)"


irc: start-irc


# ============================================================
# BRIDGE GUARDS
# ============================================================

check-docker-bridge:
	@if command -v docker >/dev/null 2>&1 && \
	   docker compose ps --status running --services 2>/dev/null | \
	   grep -qx "$(DOCKER_SERVICE)"; then \
		echo "[ERROR] Docker bridge is already running."; \
		echo "[ERROR] Do not run local and Docker bridges together."; \
		echo ""; \
		echo "Stop Docker first with:"; \
		echo "  make docker-down"; \
		exit 1; \
	fi


check-local-bridge:
	@if pgrep -f "$(BRIDGE_ABS)" >/dev/null 2>&1; then \
		echo "[ERROR] A local bridge.py instance is already running."; \
		echo "[ERROR] Starting Docker would create two Telegram getUpdates clients."; \
		echo ""; \
		echo "Stop the local bridge first."; \
		exit 1; \
	fi


# ============================================================
# LOCAL BRIDGE
# ============================================================

bridge: check check-docker-bridge
	@if [ ! -x "$(PYTHON)" ]; then \
		echo "[ERROR] Python environment not found."; \
		echo "Run:"; \
		echo "  make setup"; \
		exit 1; \
	fi

	@if pgrep -f "$(BRIDGE_ABS)" >/dev/null 2>&1; then \
		echo "[ERROR] bridge.py is already running locally."; \
		exit 1; \
	fi

	@echo ""
	@echo "[BRIDGE] Starting in $(START_DELAY) seconds..."
	@sleep $(START_DELAY)

	@$(PYTHON) "$(BRIDGE)"


run: check
	@if [ ! -x "$(PYTHON)" ]; then \
		$(MAKE) setup; \
	fi

	@$(MAKE) check-docker-bridge
	@$(MAKE) start-irc
	@$(MAKE) bridge


local: run


# ============================================================
# DOCKER
# ============================================================

docker-build: check
	@command -v docker >/dev/null 2>&1 || \
		(echo "[ERROR] Docker is not installed"; exit 1)

	@docker compose version >/dev/null 2>&1 || \
		(echo "[ERROR] Docker Compose is unavailable"; exit 1)

	@echo "[DOCKER] Building bridge image..."
	@$(COMPOSE) build


docker-up: check check-local-bridge
	@command -v docker >/dev/null 2>&1 || \
		(echo "[ERROR] Docker is not installed"; exit 1)

	@if $(COMPOSE) ps --status running --services 2>/dev/null | \
	    grep -qx "$(DOCKER_SERVICE)"; then \
		echo "[DOCKER] Bridge is already running."; \
		exit 0; \
	fi

	@echo "[DOCKER] Starting bridge..."
	@$(COMPOSE) up -d

	@echo "[OK] Docker bridge started."


docker-down:
	@if command -v docker >/dev/null 2>&1 && \
	   docker compose version >/dev/null 2>&1; then \
		echo "[DOCKER] Stopping Compose stack..."; \
		$(COMPOSE) down; \
	else \
		echo "[DOCKER] Docker Compose unavailable."; \
	fi


docker: check-local-bridge docker-build docker-up


logs:
	@$(COMPOSE) logs -f "$(DOCKER_SERVICE)"


down: docker-down


# ============================================================
# STATUS
# ============================================================

status:
	@echo ""
	@echo "Telegram <-> IRC Bridge status"
	@echo "==============================="
	@echo ""

	@if [ -f "$(IRC_PID)" ]; then \
		PID=$$(cat "$(IRC_PID)" 2>/dev/null || true); \
		if [ -n "$$PID" ] && kill -0 "$$PID" 2>/dev/null; then \
			echo "[IRC]    Make-managed server: RUNNING (PID $$PID)"; \
		else \
			echo "[IRC]    Make-managed PID: STALE"; \
		fi; \
	elif command -v ss >/dev/null 2>&1 && \
	     ss -ltn | awk '{print $$4}' | grep -Eq '(^|:)$(IRC_PORT)$$'; then \
		echo "[IRC]    External/manual server: RUNNING on $(IRC_PORT)"; \
	else \
		echo "[IRC]    STOPPED"; \
	fi

	@if pgrep -f "$(BRIDGE_ABS)" >/dev/null 2>&1; then \
		echo "[LOCAL]  Bridge: RUNNING"; \
	else \
		echo "[LOCAL]  Bridge: STOPPED"; \
	fi

	@if command -v docker >/dev/null 2>&1 && \
	   $(COMPOSE) ps --status running --services 2>/dev/null | \
	   grep -qx "$(DOCKER_SERVICE)"; then \
		echo "[DOCKER] Bridge: RUNNING"; \
	else \
		echo "[DOCKER] Bridge: STOPPED"; \
	fi

	@echo ""


# ============================================================
# STOP / RESTART
# ============================================================

stop:
	@echo "[STOP] Stopping Make/Docker managed services..."
	@$(MAKE) docker-down
	@$(MAKE) stop-irc
	@echo ""
	@echo "[NOTE] A manually started ./start process is not killed automatically."
	@echo "[NOTE] Stop it from its terminal with Ctrl+C."
	@echo "[OK] Managed services stopped."


restart:
	@$(MAKE) stop
	@$(MAKE) run


# ============================================================
# CLEANUP
# ============================================================

clean:
	@$(MAKE) stop
	@rm -rf __pycache__
	@rm -f "$(IRC_LOG)"
	@echo "[OK] Temporary files removed."


fclean: clean
	@echo "[CLEAN] Removing Python environment..."
	@rm -rf "$(VENV)"

	@echo "[CLEAN] Cleaning IRC server..."
	@if [ -d "$(IRC_DIR)" ]; then \
		$(MAKE) -C "$(IRC_DIR)" fclean; \
	fi

	@if command -v docker >/dev/null 2>&1 && \
	   docker compose version >/dev/null 2>&1; then \
		$(COMPOSE) down \
			--rmi local \
			--volumes \
			--remove-orphans 2>/dev/null || true; \
	fi

	@echo "[OK] Full clean complete."


re: fclean
	@$(MAKE) run
