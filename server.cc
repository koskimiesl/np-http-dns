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
    pthread_cond_t condv; // condition of interest: queue has items
};

thread_id_queue* join_queue; // thread IDs of completed request processing threads

void* process_request(void* fd);
void* clean_completed(void* arg);

int main(int argc, char *argv[])
{
	thread_id_queue queue;
	queue.mutex = PTHREAD_MUTEX_INITIALIZER;
	queue.condv = PTHREAD_COND_INITIALIZER;
	join_queue = &queue;

	int listenfd;
	if ((listenfd = create_and_listen(6000)) < 0)
		return -1;

	pthread_t cleanerid;
	if ((errno = pthread_create(&cleanerid, NULL, clean_completed, NULL)) != 0)
	{
		perror("pthread_create");
		return -1;
	}
	std::cout << "started cleaner thread with ID " << cleanerid << std::endl;

	while (1)
	{
		/* accept new connection and start new thread to serve client's request */
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

		pthread_t tid;
		int* connfdptr = new int; // to ensure the value hasn't changed when accessed by the new thread
		*connfdptr = connfd;
		if ((errno = pthread_create(&tid, NULL, process_request, connfdptr)) != 0)
		{
			perror("pthread_create");
			return -1;
		}
		std::cout << "started request processing thread with ID " << tid << std::endl;
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

	if ((errno = pthread_mutex_lock(&join_queue->mutex)) != 0)
	{
		perror("pthread_mutex_lock");
		return fd;
	}
	join_queue->queue.push(pthread_self());
	pthread_cond_signal(&join_queue->condv); // inform that there are threads to clean
	if ((errno = pthread_mutex_unlock(&join_queue->mutex)) != 0)
		perror("pthread_mutex_unlock");

	return fd;
}

/* thread routine for cleaning completed request processing threads */
void* clean_completed(void* arg)
{
	while(1)
	{
		/* call pthread_join for completed request processing threads, clean up resources */
		if (pthread_mutex_lock(&join_queue->mutex) != 0)
		{
			std::cerr << "failed to lock mutex" << std::endl;
			return arg;
		}
		while (!join_queue->queue.empty()) // if queue has items, do cleaning immediately
		{
			void* result;
			if (pthread_join(join_queue->queue.front(), &result) != 0)
			{
				std::cerr << "failed to wait for thread termination" << std::endl;
				return arg;
			}
			int* fdptr = (int*)result;
			std::cout << "thread with ID " << join_queue->queue.front() << " terminated (with fd " << *fdptr << ")" << std::endl;
			delete fdptr;
			join_queue->queue.pop();
		}
		while (join_queue->queue.empty()) // if queue is empty, wait for condition (i.e. when queue has items)
		{
			pthread_cond_wait(&join_queue->condv, &join_queue->mutex);
			std::cout << "cleaner thread: condition signal received" << std::endl;
			while (!join_queue->queue.empty())
			{
				void* result;
				if (pthread_join(join_queue->queue.front(), &result) != 0)
				{
					std::cerr << "failed to wait for thread termination" << std::endl;
					return arg;
				}
				int* fdptr = (int*)result;
				std::cout << "thread with ID " << join_queue->queue.front() << " terminated (with fd " << *fdptr << ")" << std::endl;
				delete fdptr;
				join_queue->queue.pop();
			}
		}
		if (pthread_mutex_unlock(&join_queue->mutex) != 0)
		{
			std::cerr << "failed to unlock mutex" << std::endl;
			return arg;
		}
	}
	return arg;
}
