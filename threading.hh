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

/* thread routine for cleaning completed request processing threads */
void* cleaner(void* queue);

thread_queue create_queue();

void enter_queue(thread_queue& queue);

int start_thread(void* (*routine)(void*), void* arg, std::string name);

#endif
