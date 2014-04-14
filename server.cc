#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "networking.hh"
#include "threading.hh"

thread_queue joinqueue; // request processing threads ready to be joined

void* process_request(void* fd);

int main(int argc, char *argv[])
{
	joinqueue = create_queue();

	int listenfd;
	if ((listenfd = create_and_listen(6001)) < 0)
		return -1;

	if (start_thread(cleaner, &joinqueue, "cleaner") < 0)
		return -1;

	while (1)
	{
		/* for each new connection, start new thread to process request */
		int connfd;
		struct sockaddr_in6	cliaddr;
		socklen_t len = sizeof(cliaddr);
		if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0)
		{
			perror("accept");
			return -1;
		}

		char buff[80];
		std::cout << "connection from " << inet_ntop(AF_INET6, &cliaddr.sin6_addr, buff, sizeof(buff))
				  << ", port " << ntohs(cliaddr.sin6_port) << ", fd is " << connfd << std::endl;

		int* connfdptr = new int; // to ensure the value hasn't changed when accessed by the started thread
		*connfdptr = connfd;
		if (start_thread(process_request, connfdptr, "process_request") < 0)
			return -1;
	}
}

/* thread routine for processing client's request */
void* process_request(void* fd)
{
	int connfd = *(int*)fd;
	std::cout << "thread " << pthread_self() << ": serving client through fd " << connfd << std::endl;
	char recvbuf[1024];
	memset(recvbuf, 0, 1024);
	int n = read(connfd, recvbuf, 1024);
	if (n < 0)
		perror("error reading from socket");
	std::cout << "thread " << pthread_self() << ": received " << n << " bytes" << std::endl;
	close(connfd); // fd number can now be reused by new connections

	enter_queue(joinqueue);
	return fd;
}
