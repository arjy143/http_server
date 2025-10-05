#include "thread_pool.h"
#include "platform.h"

thread_pool_t* thread_pool_create(int num_threads, int queue_capacity)
{
    thread_pool_t* pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (!pool)
    {
        return NULL;
    }

    pool->queue = (int*)malloc(sizeof(int) * queue_capacity);
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
void thread_pool_destroy(thread_pool_t* pool);
int thread_pool_add_task(thread_pool_t* pool, int client_fd);

void* worker_thread(void* arg)
{
    thread_pool_t* pool = (thread_pool_t*)arg;
    while(1)
    {
        pthread_mutex_lock(&pool->mutex);
        while (pool->count == 0 && !pool->stop)
        {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        if (pool->stop)
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        //popping connection from queue
        int client_fd = pool->queue[pool->front];
        pool->front = (pool->front + 1)%pool->capacity;
        pool->count--;
        pthread_mutex_unlock(&pool->mutex);

        
    }
}