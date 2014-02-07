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

#define RECVBUFSIZE 1000

int main(int argc, char *argv[])
{
	int sockfd, sent, recvd;
	char buffer[RECVBUFSIZE];

	if (argc < 6)
	{
		std::cerr << "Usage: ./client <method> <filename> <hostname> <service> <username>" << std::endl;
		return -1;
	}

	sockfd = tcp_connect(argv[3], argv[4]);

	if (sockfd < 0)
		return -1;

	http_message httpmsg(http_method::GET, argv[2], argv[3], argv[5]);
	std::string message = httpmsg.create_request();
	std::cout << "Sending message:" << std::endl << message << std::endl;

	if ((sent = send_message(sockfd, message)) < 0)
		return -1;
	std::cout << sent << " bytes sent" << std::endl;

	// first read response header
	bool emptylinefound = false;
	std::string readtotal; // total message read from socket so far
	std::string delimiter("\r\n\r\n");
	std::string header;
	while ((recvd = read(sockfd, buffer, RECVBUFSIZE)) > 0 && !emptylinefound)
	{
		std::string chunk(buffer, recvd);
		readtotal += chunk;
		size_t found = readtotal.find(delimiter);
		if (found != std::string::npos)
		{
			emptylinefound = true;
			std::cout << "found at " << found << std::endl;
			std::cout << readtotal << std::endl;
			header = readtotal.substr(0, found + delimiter.length()); // include empty line to header
		}
	}
	if (recvd < 0)
	{
		perror("Error reading from socket");
		return -1;
	}
	if (!emptylinefound)
	{
		std::cerr << "Empty line not found" << std::endl;
		return -1;
	}
	http_message response;
	if (!response.parse_resp_header(header))
		std::cerr << "Parsing failed" << std::endl;
	else
	{
		if (response.status_code == http_status_code::_200_OK_)
			std::cout << "OK!" << std::endl;
	}
	response.dump_values();
	return 0;
}
