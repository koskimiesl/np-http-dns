#include <iostream>
#include <syslog.h>
#include <unistd.h>

#include "daemon.hh"
#include "general.hh"
#include "http.hh"
#include "networking.hh"
#include "threading.hh"

thread_queue joinqueue; // request processing threads ready to be joined

void* process_request(void* parameters);

/*
 * Main function
 */
int main(int argc, char *argv[])
{
	unsigned short port;
	bool debug = false; // becomes a daemon by default
	std::string servpath; // path to serving directory
	std::string dnsservip; // DNS server to use
	std::string username;
	if (get_server_opts(argc, argv, port, debug, servpath, dnsservip, username) < 0)
		return -1;

	if (!debug)
	{
		std::cout << "starting server as daemon..." << std::endl;
		if (daemon_init("httpserver") < 0)
			return -1;
		syslog(LOG_NOTICE, "started");
		closelog();
	}

	/* create serving directory if it doesn't exist */
	if (create_dir(servpath) < 0)
		return -1;

	joinqueue = create_queue();

	int listenfd;
	if ((listenfd = create_and_listen(port)) < 0)
		return -1;

	if (start_thread(cleaner, &joinqueue, "cleaner") < 0)
		return -1;

	while (1)
	{
		std::cout << "listening new connections" << std::endl;

		/* accept new client connection */
		int connfd;
		if ((connfd = accept_connection(listenfd)) < 0)
			return -1;

		/* init thread parameters */
		process_req_params* parameters = new process_req_params;
		parameters->connfd = connfd;
		parameters->servpath = servpath;
		parameters->dnsservip = dnsservip;
		parameters->username = username;
		parameters->errors = false;

		/* start new thread to process client's request */
		if (start_thread(process_request, parameters, "process_request") < 0)
			return -1;
	}
}

/*
 * Thread routine for processing client's request
 *
 * parameters: request processing parameters
 */
void* process_request(void* parameters)
{
	process_req_params* params = (process_req_params*)parameters;

	/* HTTP configuration instance for thread */
	const http_conf conf(params->dnsservip);

	try
	{
		/* read request header from socket */
		http_request request = http_request::receive_header(conf, params->connfd);
		request.print_header();

		/* process request and form response header */
		http_response response = http_response::proc_req_form_header(conf, params->connfd, request, params->servpath, params->username);
		response.print_header();

		/* write response to socket */
		if (!response.send(params->connfd, params->servpath))
		{
			std::cerr << "failed to send response" << std::endl;
			params->errors = true;
		}
	}
	catch (const general_exception& e)
	{
		std::cerr << e.what() << std::endl;
		params->errors = true;
	}

	if (params->errors)
	{
		/* try to write 404 Not Found as a general error to socket */
		http_response response = http_response::form_404_header(conf, params->username);
		response.print_header();
		if (!response.send(params->connfd, params->servpath))
			std::cerr << "failed to send general error response" << std::endl;
	}

	/* to avoid "connection reset by peer" errors in the client */
	char buf[100];
	if (read(params->connfd, buf, 100) < 0)
	{
		perror("read");
		params->errors = true;
	}

	/* now it's safe to close the socket */
	if (close(params->connfd) < 0)
	{
		perror("close");
		params->errors = true;
	}

	/* thread is now ready to be joined */
	enter_queue(joinqueue);

	return params;
}
