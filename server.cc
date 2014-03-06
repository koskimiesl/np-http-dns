#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#define LISTENQLEN 5
#define BUFSIZE 1024

int main()
{
	int listenfd, connfd;
	socklen_t len;
	struct sockaddr_in	servaddr, cliaddr;
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

	// start waiting for incoming connections, until user interrupts
	while (1)
	{
		len = sizeof(cliaddr);

		// wait for incoming connection
		// new socket fd will be used in return
		if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0)
		{
			perror("accept");
			return -1;
		}
		printf("connection from %s, port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));

		memset(recvbuf, 0, BUFSIZE);
		int n = read(connfd, recvbuf, BUFSIZE);
		if (n < 0)
			perror("Error reading from socket");
		printf("server received %d bytes: %s", n, recvbuf);

		// close accepted client socket and start waiting for new client
		// note: listenfd will remain open
		close(connfd);
	}
}
