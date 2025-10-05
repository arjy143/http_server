#include "thread_pool.h"
#include "platform.h"
#include "platform_threading.h"

thread_return_t worker_thread(thread_arg_t arg)
{
    thread_pool_t* pool = (thread_pool_t*)arg;
    while(1)
    {
        MUTEX_LOCK(&pool->mutex);
        while (pool->count == 0 && !pool->stop)
        {
            COND_WAIT(&pool->cond, &pool->mutex);
        }
        if (pool->stop)
        {
            MUTEX_UNLOCK(&pool->mutex);
            break;
        }

        //popping connection from queue
        task_t task = pool->queue[pool->front];
        pool->front = (pool->front + 1)%pool->capacity;
        pool->count--;
        MUTEX_UNLOCK(&pool->mutex);

        //call the task's function with its argument
        task.function(task.arg);
    }

    #ifdef _WIN32
        return 0;
    #else
        return NULL;
    #endif
}


thread_pool_t* thread_pool_create(int num_threads, int queue_capacity)
{
    thread_pool_t* pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (!pool)
    {
        return NULL;
    }

    pool->queue = (task_t*)malloc(sizeof(task_t) * queue_capacity);
    if (!pool->queue)
    {
        free(pool);
        return NULL;
    }
    pool->capacity = queue_capacity;
    pool->front = 0;
    pool->back = 0;
    pool->count = 0;
    pool->num_threads = num_threads;
    pool->stop = 0;
    
    for (int i = 0; i < num_threads; i++)
    {
        THREAD_CREATE(&pool->threads[i], worker_thread, pool);
    }
}
void thread_pool_destroy(thread_pool_t* pool)
{
    if (!pool)
    {
        return;
    }
    MUTEX_LOCK(&pool->mutex);
    pool->stop = 1;
    COND_BROADCAST(&pool->cond);
    MUTEX_UNLOCK(&pool->mutex);

    for (int i = 0; i < pool->num_threads; i++)
    {
        THREAD_JOIN(pool->threads[i]);
    }

    free(pool->threads);
    free(pool->queue);
    MUTEX_DESTROY(&pool->mutex);
    COND_DESTROY(&pool->cond);
    free(pool);
}

int thread_pool_add_task(thread_pool_t* pool, void* (*function)(void*), void* arg)
{
    if (!pool || !function)
    {
        return -1;
    }

    MUTEX_LOCK(&pool->mutex);
    //if the queue is full, reject the task
    if (pool->count == pool->capacity)
    {
        MUTEX_UNLOCK(&pool->mutex);
        return -1; //queue full
    }

    //otherwise task to queue
    pool->queue[pool->back].function = function;
    pool->queue[pool->back].arg = arg;
    pool->back = (pool->back + 1) % pool->capacity;
    pool->count++;
    COND_SIGNAL(&pool->cond);
    MUTEX_UNLOCK(&pool->mutex);
    return 0;
}

