#include <iostream>
#include <syslog.h>
#include <unistd.h>

#include "daemon.hh"
#include "general.hh"
#include "http.hh"
#include "networking.hh"
#include "threading.hh"

thread_queue joinqueue; // request processing threads ready to be joined

void* process_request(void* fd);

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
		std::cout << "starting daemon..." << std::endl;
		if (daemon_init("httpserver", LOG_WARNING) < 0)
			return -1;
		syslog(LOG_NOTICE, "started");
		closelog();
	}

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

		/* start new thread to process client's request */
		if (start_thread(process_request, parameters, "process_request") < 0)
			return -1;
	}
}

/* thread routine for processing client's request */
void* process_request(void* params)
{
	process_req_params parameters = *(process_req_params*)params;

	/* read request from socket */
	http_request request = http_request::from_socket(parameters.connfd);
	request.print();

	/* create response based on the request */
	http_response response = http_response::from_request(request, parameters.username);
	response.print();

	close(parameters.connfd);

	enter_queue(joinqueue);
	return params;
}
