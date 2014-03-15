/*
 * Simple HTTP client supporting GET and PUT with plain text payload
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

	if (argc != 6)
	{
		std::cerr << "Usage: ./client <method> <filename> <hostname> <service> <username>" << std::endl;
		return -1;
	}

	// allow method in lower or upper case
	std::string method(argv[1]);
	std::transform(method.begin(), method.end(), method.begin(), ::toupper);

	http_message request;

	if (method == "GET")
	{
		request = http_message(http_method::GET, argv[2], argv[3], argv[5]);
		std::string message = request.create_header();

		sockfd = tcp_connect(argv[3], argv[4]);
		if (sockfd < 0)
			return -1;

		if ((sent = send_message(sockfd, message)) < 0)
			return -1;
	}
	else if (method == "PUT")
	{
		std::ifstream file(argv[2]);
		if (!file.good())
		{
			std::cerr << "File error: " << argv[2] << std::endl;
			return -1;
		}

		// determine file size
		file.seekg(0, file.end);
		int length = file.tellg();
		file.seekg(0, file.beg);

		char* payload = new char[length]; // allocate memory for payload
		file.read(payload, length);
		if (file.gcount() != length)
		{
			std::cerr << "Failed to read file into memory" << std::endl;
			return -1;
		}
		std::cout << "Successfully read " << file.gcount() << " characters into memory" << std::endl;
		file.close();
		request = http_message(http_method::PUT, argv[2], length, payload, argv[3], argv[5]);
		std::string header = request.create_header();

		sockfd = tcp_connect(argv[3], argv[4]);
		if (sockfd < 0)
			return -1;

		if ((sent = send_message(sockfd, header, payload, length)) < 0)
			return -1;
		delete[] payload; // free memory
	}
	else
	{
		std::cerr << "'" << method << "' method not supported" << std::endl;
		return -1;
	}

	std::cout << sent << " bytes sent" << std::endl;


	// handle response
	char buffer[RECVBUFSIZE];
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
	{
		std::cerr << "Failed to parse header" << std::endl;
		return -1;
	}

	if (request.method == http_method::GET)
	{
		switch (response.status_code)
		{
		case http_status_code::_200_OK_:
		{
			std::string payload_so_far = readtotal.substr(found + delimiter.length());
			size_t payload_read = payload_so_far.length();
			std::ofstream file;
			file.open(request.filename.c_str());
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
			break;
		}
		case http_status_code::_404_NOT_FOUND_:
			std::cout << "File not found from server" << std::endl;
			break;
		default:
			std::cout << "Error occured" << std::endl;
			break;
		}
	}
	else if (request.method == http_method::PUT)
	{
		switch (response.status_code)
		{
		case http_status_code::_200_OK_:
			std::cout << "File updated successfully on server" << std::endl;
			break;
		case http_status_code::_201_CREATED_:
			std::cout << "File created successfully on server" << std::endl;
			break;
		default:
			std::cout << "Error occured" << std::endl;
			break;
		}
	}

	if (close(sockfd) < 0)
		perror("Error closing socket");

	return 0;
}
