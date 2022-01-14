#include "TaskQueue.h"
#include "ThreadPool.h"
#include <string.h>
#include <string>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
using namespace std;

ThreadPool::ThreadPool(int min, int max)
{
    //实例化任务队列
    do
    {
        taskQ = new TaskQueue;
        if (taskQ == nullptr)
        {
            std::cout << "malloc taskQ failed" << endl;
            break;
        }

        thread_ids = new pthread_t[max];
        if (thread_ids == nullptr)
        {
            std::cout << "malloc thread_ids failed" << endl;
            break;
        }

        memset(thread_ids, 0, sizeof(pthread_t) * max);
        min_num = min;
        max_num = max;
        busy_num = 0;
        live_num = min;
        exit_num = 0;

        if (pthread_mutex_init(&mutex_pool, NULL) != 0 ||
            pthread_cond_init(&not_empty, NULL) != 0)
        {
            std::cout << "mutex or condition init fail" << endl;
            break;
        }

        shutdown = false;

        //创建管理者线程
        pthread_create(&manager_id, NULL, manager, this);

        //创建工作线程
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&thread_ids[i],
                           NULL, worker, this);
        }

        return;
    } while (0);

    //释放资源
    if (thread_ids)
        delete[] thread_ids;
    if (taskQ)
        delete taskQ;
}

ThreadPool::~ThreadPool()
{
    shutdown = true;
    pthread_join(manager_id, NULL);

    for (int i = 0; i < live_num; i++)
    {
        pthread_cond_signal(&not_empty);
    }

    if (taskQ)
        delete taskQ;
    if (thread_ids)
        delete[] thread_ids;

    pthread_mutex_destroy(&mutex_pool);
    pthread_cond_destroy(&not_empty);
}

void ThreadPool::addTask(Task task)
{
    if (shutdown)
    {
        return;
    }
    taskQ->addTask(task);
    pthread_cond_signal(&not_empty);
}

int ThreadPool::getBusyNumber()
{
    pthread_mutex_lock(&mutex_pool);
    int busy_num = this->busy_num;
    pthread_mutex_unlock(&mutex_pool);
    return busy_num;
}
int ThreadPool::getLiveNumber()
{
    pthread_mutex_lock(&mutex_pool);
    int live_num = this->live_num;
    pthread_mutex_unlock(&mutex_pool);
    return live_num;
}

void *ThreadPool ::worker(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);

    while (true)
    {
        pthread_mutex_lock(&pool->mutex_pool);
        while (pool->taskQ->getNumber() == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->not_empty, &pool->mutex_pool);
            if (pool->exit_num > 0)
            {
                pool->exit_num--;
                if (pool->live_num > pool->min_num)
                {
                    pool->live_num--;
                    pthread_mutex_unlock(&pool->mutex_pool);
                    pool->thread_exit();
                }
            }
        }

        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutex_pool);
            pool->thread_exit();
        }
        Task _task = pool->taskQ->getTask();

        pool->busy_num++;
        pthread_mutex_unlock(&pool->mutex_pool);

        std::cout << "thread " << to_string(pthread_self()) << " start working\n " << endl;

        _task.function(_task.arg);
        delete _task.arg;
        _task.arg = nullptr;

        std::cout << "thread " << to_string(pthread_self()) << " end working\n " << endl;
        pthread_mutex_lock(&pool->mutex_pool);
        pool->busy_num--;
        pthread_mutex_unlock(&pool->mutex_pool);
    }

    return nullptr;
}

void *ThreadPool::manager(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    while (!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutex_pool);
        int queue_size = pool->taskQ->getNumber();
        int live_num = pool->live_num;
        int busy_num = pool->busy_num;
        pthread_mutex_unlock(&pool->mutex_pool);

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

void ThreadPool::thread_exit()
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < max_num; i++)
    {
        if (thread_ids[i] == tid)
        {
            thread_ids[i] = 0;
            cout << "thread_exit called,"
                 << to_string(tid) << " exiting" << endl;
            break;
        }
    }
    pthread_exit(NULL);
}