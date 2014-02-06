/*
 * HTTP DNS client
 */
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

#include "http.hh"
#include "networking.hh"

#define RECVBUFSIZE 100

int main(int argc, char *argv[])
{
	int sockfd, n;
	char buf[RECVBUFSIZE] = { 0 };

	if (argc < 6)
	{
		std::cerr << "Usage: ./client <method> <filename> <hostname> <service> <username>" << std::endl;
		return -1;
	}

	sockfd = tcp_connect(argv[3], argv[4]);

	if (sockfd < 0)
		return -1;

	http_message httpmsg(http_method::GET, argv[2], argv[3], argv[5]);
	std::string message = httpmsg.to_string();
	std::cout << "Sending message:" << std::endl << message << std::endl;

	n = send_message(sockfd, message);
	std::cout << n << " bytes sent" << std::endl;

	// first read response header
	size_t buf_idx = 0;
	while (buf_idx < RECVBUFSIZE && 1 == read(sockfd, &buf[buf_idx], 1))
	{
	    if (buf_idx > 0          &&
	        '\n' == buf[buf_idx] &&
	        '\n' == buf[buf_idx - 1])
	    {
	    	std::cout << "emtpy line.." << std::endl;
	    	break;
	    }
	    buf_idx++;
	}
	if (n < 0)
	{
		perror("read error");
		return -1;
	}

	return 0;
}
