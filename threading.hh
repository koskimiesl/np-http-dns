/* Threading related functionalities */

#ifndef NETPROG_THREADING_HH
#define NETPROG_THREADING_HH

#include <queue>
#include <pthread.h>
#include <string>

/* for inter-thread communication, access protected by mutex */
struct thread_queue
{
    std::queue<pthread_t> queue; // thread IDs
    pthread_mutex_t mutex;
    pthread_cond_t condv; // condition of interest: queue has items
};

/* parameters passed to each request processing thread */
struct process_req_params
{
	int connfd;
	std::string username;
	std::string servpath;
	bool errors; // true if errors occured in thread routine
};

/* thread routine for cleaning completed request processing threads */
void* cleaner(void* queue);

thread_queue create_queue();

void enter_queue(thread_queue& queue);

int start_thread(void* (*routine)(void*), void* arg, std::string name);

#endif
