#ifndef THREAD_POOL_H
#define THREAD_POOL_H

//using my custom threading interface
#include "platform_threading.h"

//generic tasks - literally just a function pointer with argument
typedef struct
{
    void* (*function)(void* arg);
    void* arg;
} task_t;

//generic thread pool to store generic tasks
typedef struct 
{
    task_t* queue;
    int capacity;
    int front;
    int back;
    int count;

    thread_t* threads;
    int num_threads;
    thread_mutex_t mutex;
    thread_cond_t cond;

    int stop;
} thread_pool_t;

thread_pool_t* thread_pool_create(int num_threads, int queue_capacity);
void thread_pool_destroy(thread_pool_t* pool);
//adds function arg pair to the pool
int thread_pool_add_task(thread_pool_t* pool, void (*function)(void*), void* arg);


#endif