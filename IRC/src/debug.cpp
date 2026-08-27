#include "../includes/debug.hpp"
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		std::cerr  << "Error: socket failed" << std::endl;
		return (1);
	}

	DBG("Socket created successfully, fd = " << server_fd);
	return (0);
}
