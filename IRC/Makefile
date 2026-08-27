NAME = ircserv

CXX = c++ -std=c++98
CXXFLAGS = -Wall -Wextra -Werror -Iincludes

DEBUG ?= 0

SRCS = src/main.cpp     \
		src/server.cpp   \
		src/client.cpp	  \
		src/Icommande.cpp  \
		src/profile.cpp		\
		src/stateServer.cpp  \
		src/configServer.cpp  \
		src/channel.cpp \
		src/ModeProcess.cpp
# src/debug.cpp

OBJS = $(SRCS:.cpp=.o)

all: banner
	$(CXX) $(CXXFLAGS) -DDEBUG=$(DEBUG) $(SRCS) -o $(NAME)

banner:
	@printf "\033[1;36m\n"
	@printf "  _____ _____   _____ \n"
	@printf " |_   _|  __ \\ / ____|\n"
	@printf "   | | | |__) | |     \n"
	@printf "   | | |  _  /| |     \n"
	@printf "  _| |_| | \\ \\| |____ \n"
	@printf " |_____|_|  \\_\\\\_____|\n"
	@printf "\033[0m\n"
	@printf "  DEBUG : $(DEBUG)\n\n"

debug:
	$(MAKE) DEBUG=1 all

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all banner debug clean fclean re
