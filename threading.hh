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
	std::string servpath;
	std::string dnsservip;
	std::string username;
	bool errors; // true if errors occured in thread routine
};

/*
 * Thread routine for cleaning completed request processing threads
 *
 * queue: join queue
 */
void* cleaner(void* queue);

/*
 * Create join queue
 *
 * return: queue structure
 */
thread_queue create_queue();

/*
 * Enter self into queue
 *
 * queue: queue to enter
 */
void enter_queue(thread_queue& queue);

/*
 * Start a thread
 *
 * routine: thread routine to run
 * arg: thread routine arguments
 * name: thread name
 * return: 0 on success, -1 on failure
 */
int start_thread(void* (*routine)(void*), void* arg, std::string name);

#endif
