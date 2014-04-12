#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <unistd.h>

#include "networking.hh"

/* for inter-thread communication, access protected by mutex */
struct thread_id_queue
{
    std::queue<pthread_t> queue;
    pthread_mutex_t mutex;
};

thread_id_queue* join_queue; // thread IDs of completed threads

void* serve_client(void*);

int main(int argc, char *argv[])
{
	thread_id_queue queue;
	queue.mutex = PTHREAD_MUTEX_INITIALIZER;
	join_queue = &queue;

	int listenfd;
	if ((listenfd = create_and_listen(6001)) < 0)
		return -1;

	int connfd;
	fd_set readset;
	struct timeval timeout;
	int numfds;
	while (1)
	{
		/* use select with timeout to see if read won't block */
		FD_ZERO(&readset);
		FD_SET(listenfd, &readset);
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		if ((numfds = select(listenfd+1, &readset, NULL, NULL, &timeout)) < 0)
		{
			perror("select");
			return -1;
		}
		if (numfds == 0)
			std::cout << "select timed out" << std::endl;
		else
		{
			connfd = -1;
			struct sockaddr_in6	cliaddr;
			socklen_t len = sizeof(cliaddr);

			/* accept connection and start new thread to serve client's request */
			if (FD_ISSET(listenfd, &readset))
			{
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0)
				{
					perror("accept");
					return -1;
				}

				char buff[80];
				std::cout << "connection from " << inet_ntop(AF_INET6, &cliaddr.sin6_addr, buff, sizeof(buff))
						  << ", port " << ntohs(cliaddr.sin6_port) << ", fd is " << connfd << std::endl;

				pthread_t tid;
				int* connfdptr = new int; // to ensure the value hasn't changed when accessed by the new thread
				*connfdptr = connfd;
				if ((errno = pthread_create(&tid, NULL, serve_client, connfdptr)) != 0)
				{
					std::cerr << "failed to start thread: " << strerror(errno) << std::endl;
					return -1;
				}
				std::cout << "started new thread with ID " << tid << std::endl;
			}
		}

		/* call pthread_join for completed threads, clean up resources */
		if (pthread_mutex_lock(&join_queue->mutex) != 0)
		{
			std::cerr << "failed to lock mutex" << std::endl;
			return -1;
		}
		while (!join_queue->queue.empty())
		{
			void* result;
			if (pthread_join(join_queue->queue.front(), &result) != 0)
			{
				std::cerr << "failed to wait for thread termination" << std::endl;
				return -1;
			}
			int* fdptr = (int*)result;
			std::cout << "thread with ID " << join_queue->queue.front() << " terminated (with fd " << *fdptr << ")" << std::endl;
			delete fdptr;
			join_queue->queue.pop();
		}
		if (pthread_mutex_unlock(&join_queue->mutex) != 0)
		{
			std::cerr << "failed to unlock mutex" << std::endl;
			return -1;
		}
	}
}

/* thread routine */
void* serve_client(void* fd)
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

	/* inform thread has completed (i.e. pthread_join can now be called) */
	if (pthread_mutex_lock(&join_queue->mutex) != 0)
	{
		std::cerr << "failed to lock mutex" << std::endl;
		return fd;
	}
	join_queue->queue.push(pthread_self());
	if (pthread_mutex_unlock(&join_queue->mutex) != 0)
		std::cerr << "failed to unlock mutex" << std::endl;

	return fd;
}
