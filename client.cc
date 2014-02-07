/*
 * HTTP DNS client
 */
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "http.hh"
#include "networking.hh"

#define RECVBUFSIZE 10000

int main(int argc, char *argv[])
{
	int sockfd, sent, recvd;
	char buffer[RECVBUFSIZE];

	if (argc != 6)
	{
		std::cerr << "Usage: ./client <method> <filename> <hostname> <service> <username>" << std::endl;
		return -1;
	}

	// allow method in lower or upper case
	std::string method(argv[1]);
	std::transform(method.begin(), method.end(), method.begin(), ::toupper);

	http_message httpmsg;
	if (method == "GET")
		httpmsg = http_message(http_method::GET, argv[2], argv[3], argv[5]);
	else if (method == "PUT")
		httpmsg = http_message(http_method::PUT, argv[2], argv[3], argv[5]);
	else
	{
		std::cerr << "Method not supported" << std::endl;
		return -1;
	}

	std::string message = httpmsg.create_request();

	sockfd = tcp_connect(argv[3], argv[4]);
	if (sockfd < 0)
		return -1;

	std::cout << std::endl << "Sending message:" << std::endl << message << std::endl;
	if ((sent = send_message(sockfd, message)) < 0)
		return -1;
	std::cout << sent << " bytes sent" << std::endl;

	// first read response header
	bool emptylinefound = false;
	std::string readtotal; // total message read from socket so far
	std::string delimiter("\r\n\r\n");
	std::string header;
	size_t found; // index of start of delimiter
	memset(buffer, 0, RECVBUFSIZE);
	while (!emptylinefound && (recvd = read(sockfd, buffer, RECVBUFSIZE)) > 0)
	{
		std::string chunk(buffer, recvd);
		readtotal += chunk;
		found = readtotal.find(delimiter);
		if (found != std::string::npos)
		{
			emptylinefound = true;
			header = readtotal.substr(0, found + delimiter.length()); // include empty line to header
			std::cout << std::endl << "Response header:" << std::endl << header << std::endl;
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
		std::cerr << "Failed to parse header" << std::endl;
	else
	{
		if (response.status_code == http_status_code::_200_OK_)
		{
			std::string payload_so_far = readtotal.substr(found + delimiter.length());
			size_t payload_read = payload_so_far.length();
			std::ofstream file;
			file.open(argv[2]);
			file << payload_so_far;

			// read rest of payload from socket and write to a file
			while (payload_read < response.content_length && (recvd = read(sockfd, buffer, RECVBUFSIZE)) > 0)
			{
				std::string chunk(buffer, recvd);
				payload_read += recvd;
				file << chunk;
			}
			file.close();
			if (recvd == 0)
			{
				std::cerr << "Server closed connection" << std::endl;
				return -1;
			}
			if (recvd < 0)
			{
				perror("Error reading from socket");
				return -1;
			}
		}
	}
	response.dump_values();
	return 0;
}
