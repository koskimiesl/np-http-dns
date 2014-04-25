/*
 * Simple HTTP client supporting GET, PUT and POST with plain text payload
 */

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

	/* connect to server */
	int sockfd;
	if ((sockfd = tcp_connect(hostname, port)) < 0)
		return -1;

	try
	{
		const http_conf conf;

		/* create request header based on command line parameters */
		http_request req = http_request::form_header(conf, method, dirpath, filename, hostname, username, queryname);
		req.print_header();

		/* send the request */
		if (!req.send(sockfd, dirpath))
			throw general_exception("failed to send the request");

		/* read response from socket */
		http_response resp = http_response::receive(conf, sockfd, req.method, dirpath, req.filename);
		resp.print_header();
	}
	catch (const general_exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	catch (const http_exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	if (close(sockfd) < 0)
	{
		perror("close");
		return -1;
	}

	return 0;
}
