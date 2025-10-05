#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct 
{
    int* queue;
    int capacity;
    int front;
    int back;
    int count;

    pthread_t* threads;
    int num_threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int stop;
} thread_pool_t;

thread_pool_t* thread_pool_create(int num_threads, int queue_capacity);
void thread_pool_destroy(thread_pool_t* pool);
int thread_pool_add_task(thread_pool_t* pool, int client_fd);


#endif