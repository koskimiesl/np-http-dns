#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daemon.hh"
#include "helpers.hh"

#define LISTENQLEN 5
#define BUFSIZE 1024

int main(int argc, char *argv[])
{
	char port[PORTLEN];
	bool debug = false; // becomes a daemon by default
	if (get_server_opts(argc, argv, port, debug) < 0)
		return -1;

	if (!debug)
	{
		std::cout << "starting daemon..." << std::endl;
		if (daemon_init("httpserver", LOG_WARNING) < 0)
			return -1;
		syslog(LOG_NOTICE, "started");
		closelog();
	}

	int listenfd, maxfd, numfds;
	int connfd = -1;
	socklen_t len;
	struct sockaddr_in	servaddr, cliaddr;
	fd_set readset; // fd set to watch to see if read won't block
	struct timeval timeout;
	char buff[80];
	char recvbuf[BUFSIZE];

	// create socket for listening
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

	// bind server address to socket
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any interface
	servaddr.sin_port = htons(5000);
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		return -1;
	}

	// set socket to passive mode
	if (listen(listenfd, LISTENQLEN) < 0)
	{
		perror("listen");
		return -1;
	}

	maxfd = listenfd + 1;

	while (1)
	{
		FD_ZERO(&readset); // clear the set
		FD_SET(listenfd, &readset); // add listen fd to the set
		if (connfd >= 0)
			FD_SET(connfd, &readset); // add client connection fd to the set

		// on Linux, select modifies timeout -> reinitialize it
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		if ((numfds = select(maxfd, &readset, NULL, NULL, &timeout)) < 0)
		{
			perror("select");
			return -1;
		}

		len = sizeof(cliaddr);
		if (FD_ISSET(listenfd, &readset))
		{
			if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0)
			{
				perror("accept");
				return -1;
			}
			std::cout << "connection from " << inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff))
					  << ", port " << ntohs(cliaddr.sin_port) << std::endl;

			if (connfd >= maxfd)
				maxfd = connfd + 1;
		}

		if (connfd >= 0 && FD_ISSET(connfd, &readset))
		{
			memset(recvbuf, 0, BUFSIZE);
			int n = read(connfd, recvbuf, BUFSIZE);
			if (n < 0)
				perror("read");
			else
				std::cout << "server received " << n << " bytes: " << std::endl << recvbuf << std::endl;

			// close client socket and start waiting for new client
			close(connfd);
			connfd = -1;
			std::cout << "closed connection" << std::endl;
		}

		if (numfds == 0)
			std::cout << "timer expired" << std::endl;
	}
}
