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

#include "general.hh"
#include "http.hh"
#include "networking.hh"

#define RECVBUFSIZE 10000

int main(int argc, char *argv[])
{
	std::string hostname, port, method, filename, username, dirpath, queryname;
	if (get_client_opts(argc, argv, hostname, port, method, filename, username, dirpath, queryname) < 0)
		return -1;

	/* create directory for files if it doesn't exist */
	if (method != "POST" && create_dir(dirpath) < 0)
		return -1;

	/* create request header based on command line parameters */
	http_request request;
	try
	{
		request = http_request::form_header(method, dirpath, filename, hostname, username, queryname);
	}
	catch (const http_exception& e)
	{
		std::cerr << "failed to create request" << std::endl;
		return -1;
	}
	request.print_header();

	/* connect to server */
	int sockfd;
	if ((sockfd = tcp_connect(hostname, port)) < 0)
		return -1;

	/* send the request */
	if (!request.send(sockfd, dirpath))
		return -1;

	/* read response from socket */
	http_response response = http_response::receive(sockfd, request.method, dirpath, request.filename);
	response.print_header();

	if (close(sockfd) < 0)
		perror("close");

	return 0;
}
