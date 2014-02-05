/*
 * HTTP DNS client
 */

#include <iostream>

#include "networking.hh"


int main(int argc, char *argv[])
{
	int sockfd;

	if (argc < 3)
	{
		std::cerr << "Usage: ./<progname> <hostname> <service>" << std::endl;
		return -1;
	}

	sockfd = tcp_connect(argv[1], argv[2]);
	std::cout << sockfd << std::endl;
	return 0;
}
