#include "threadpool.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

const int NUMBER = 2;

threadpool *threadpool_create(int min, int max,
                              int queue_capacity)
{
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));
    do
    {
        if (pool == NULL)
        {
            printf("malloc threadpool failed\n");
            break;
        }

        pool->thread_ids = (pthread_t *)malloc(
            sizeof(pthread_t) * max);
        if (pool->thread_ids == NULL)
        {
            printf("malloc thread_ids failed\n");
            break;
        }

        memset(pool->thread_ids, 0, sizeof(pthread_t) * max);
        pool->min_num = min;
        pool->max_num = max;
        pool->busy_num = 0;
        pool->live_num = min;
        pool->exit_num = 0;

        if (pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutex_busy, NULL) != 0 ||
            pthread_cond_init(&pool->not_empty, NULL) != 0 ||
            pthread_cond_init(&pool->not_full, NULL) != 0)
        {
            printf("mutex or condition init fail");
            break;
        }

        pool->taskQ = (task *)malloc(
            sizeof(task) * queue_capacity);
        pool->queue_capacity = queue_capacity;
        pool->queue_size = 0;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = 0;

        //创建管理者线程
        pthread_create(&pool->manager_id, NULL, manager, pool);

        //创建工作线程
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&pool->thread_ids[i],
                           NULL, worker, pool);
        }

        return pool;
    } while (0);

    //释放资源
    if (pool && pool->thread_ids)
        free(pool->thread_ids);
    if (pool && pool->taskQ)
        free(pool->taskQ);
    if (pool)
        free(pool);

    return NULL;
}

int threadpool_destroy(threadpool *pool)
{
    if (pool == NULL)
        return -1;

    pool->shutdown = 1;
    pthread_join(pool->manager_id, NULL);

    for (int i = 0; i < pool->live_num; i++)
    {
        pthread_cond_signal(&pool->not_empty);
    }

    if (pool->taskQ)
        free(pool->taskQ);
    if (pool->thread_ids)
        free(pool->thread_ids);

    pthread_mutex_destroy(&pool->mutex_pool);
    pthread_mutex_destroy(&pool->mutex_busy);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);

    free(pool);
    pool = NULL;

    return 0;
}

void threadpool_add(threadpool *pool,
                    void (*func)(void *), void *arg)
{
    pthread_mutex_lock(&pool->mutex_pool);
    while (pool->queue_size == pool->queue_capacity &&
           !pool->shutdown)
    {
        pthread_cond_wait(&pool->not_full, &pool->mutex_pool);
    }

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutex_pool);
        return;
    }

    pool->taskQ[pool->queue_rear].function = func;
    pool->taskQ[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) %
                       pool->queue_capacity;
    pool->queue_size++;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->mutex_pool);
}

int threadpool_busy_num(threadpool *pool)
{
    pthread_mutex_lock(&pool->mutex_busy);
    int busy_num = pool->busy_num;
    pthread_mutex_unlock(&pool->mutex_busy);
    return busy_num;
}

int threadpool_live_num(threadpool *pool)
{
    pthread_mutex_lock(&pool->mutex_pool);
    int live_num = pool->live_num;
    pthread_mutex_unlock(&pool->mutex_pool);
    return live_num;
}

void *worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex_pool);
        while (pool->queue_size == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->not_empty, &pool->mutex_pool);
            if (pool->exit_num > 0)
            {
                pool->exit_num--;
                if (pool->live_num > pool->min_num)
                {
                    pool->live_num--;
                    pthread_mutex_unlock(&pool->mutex_pool);
                    thread_exit(pool);
                }
            }
        }

        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutex_pool);
            thread_exit(pool);
        }
        task _task;
        _task.function = pool->taskQ[pool->queue_front].function;
        _task.arg = pool->taskQ[pool->queue_front].arg;

        pool->queue_front = (pool->queue_front + 1) %
                            pool->queue_capacity;
        pool->queue_size--;

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->mutex_pool);

        printf("thread %ld start working\n", pthread_self());
        pthread_mutex_lock(&pool->mutex_pool);
        pool->busy_num++;
        pthread_mutex_unlock(&pool->mutex_pool);
        _task.function(_task.arg);
        free(_task.arg);
        _task.arg = NULL;

        printf("thread %ld end working\n", pthread_self());
        pthread_mutex_lock(&pool->mutex_pool);
        pool->busy_num--;
        pthread_mutex_unlock(&pool->mutex_pool);
    }

    return NULL;
}

void *manager(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    while (!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutex_pool);
        int queue_size = pool->queue_size;
        int live_num = pool->live_num;
        pthread_mutex_unlock(&pool->mutex_pool);

        pthread_mutex_lock(&pool->mutex_busy);
        int busy_num = pool->busy_num;
        pthread_mutex_unlock(&pool->mutex_busy);

        if (queue_size > live_num && live_num < pool->max_num)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            int counter = 0;
            for (int i = 0; i < pool->max_num &&
                            counter < NUMBER &&
                            pool->live_num < pool->max_num;
                 i++)
            {
                if (pool->thread_ids[i] == 0)
                {
                    pthread_create(&pool->thread_ids[i], NULL, worker, pool);
                    counter++;
                    pool->live_num++;
                }
            }
            pthread_mutex_unlock(&pool->mutex_pool);
        }

        if (busy_num * 2 < live_num && live_num > pool->min_num)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            pool->exit_num = NUMBER;
            pthread_mutex_unlock(&pool->mutex_pool);

            for (int i = 0; i < NUMBER; i++)
                pthread_cond_signal(&pool->not_empty);
        }
    }
    return NULL;
}

void thread_exit(threadpool *pool)
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->max_num; i++)
    {
        if (pool->thread_ids[i] == tid)
        {
            pool->thread_ids[i] = 0;
            printf("thread_exit called,%ld exiting\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
}