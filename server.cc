#include <iostream>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daemon.hh"
#include "general.hh"
#include "http.hh"
#include "networking.hh"
#include "threading.hh"

thread_queue joinqueue; // request processing threads ready to be joined

void* process_request(void* parameters);

int main(int argc, char *argv[])
{
	unsigned short port;
	bool debug = false; // becomes a daemon by default
	std::string servpath; // path to serving directory
	std::string username;
	if (get_server_opts(argc, argv, port, debug, servpath, username) < 0)
		return -1;

	if (!debug)
	{
		std::cout << "starting server as daemon..." << std::endl;
		if (daemon_init("httpserver", LOG_WARNING) < 0)
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
		parameters->username = username;
		parameters->servpath = servpath;
		parameters->errors = false;

		/* start new thread to process client's request */
		if (start_thread(process_request, parameters, "process_request") < 0)
			return -1;
	}
}

/* thread routine for processing client's request */
void* process_request(void* parameters)
{
	process_req_params params = *(process_req_params*)parameters;

	/* HTTP configuration instance for thread */
	const http_conf conf;

	/* read request header from socket */
	http_request request = http_request::receive_header(conf, params.connfd);
	request.print_header();

	/* process request and form response header */
	http_response response = http_response::proc_req_form_header(conf, params.connfd, request, params.servpath, params.username);
	response.print_header();

	/* write response to socket */
	if (!response.send(params.connfd, params.servpath))
		params.errors = true;

	/* to reduce "connection reset by peer" errors in the receiving end */
	if (shutdown(params.connfd, SHUT_RDWR) < 0)
	{
		perror("shutdown");
		params.errors = true;
	}

	if (close(params.connfd) < 0)
	{
		perror("close");
		params.errors = true;
	}

	enter_queue(joinqueue);
	return parameters;
}
