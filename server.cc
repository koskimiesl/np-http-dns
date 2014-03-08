#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daemon.hh"

#define LISTENQLEN 5
#define BUFSIZE 1024

int main()
{
	daemon_init("httpserver", LOG_WARNING);
	syslog(LOG_NOTICE, "First daemon started.");
	sleep(30);
	syslog (LOG_NOTICE, "First daemon terminating...");
	closelog();
	return 0;

	/*
	int listenfd, maxfd, numfds;
	int connfd = -1;
	socklen_t len;
	struct sockaddr_in	servaddr, cliaddr;
	fd_set readset; // file descriptors to watch to see if read won't block
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

		// on Linux, select modifies timeout -> reinitialize
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
			printf("connection from %s, port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));

			if (connfd >= maxfd)
				maxfd = connfd + 1;
		}

		if (connfd >= 0 && FD_ISSET(connfd, &readset))
		{
			memset(recvbuf, 0, BUFSIZE);
			int n = read(connfd, recvbuf, BUFSIZE);
			if (n < 0)
				perror("Error reading from socket");
			else
				printf("server received %d bytes: %s", n, recvbuf);

			// close accepted client socket and start waiting for new client
			// note: listenfd will remain open
			close(connfd);
			connfd = -1;
			printf("closed connection\n");
		}

		if (numfds == 0)
			printf("Timer expired\n");
	}
	*/
}
