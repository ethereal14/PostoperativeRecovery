#pragma once

#include "TaskQueue.h"

class ThreadPool
{
public:
    ThreadPool(int min, int max);
    ~ThreadPool();
    void addTask(Task task);
    int getBusyNumber();
    int getLiveNumber();

private:
    static void *worker(void *arg);
    static void *manager(void *arg);
    void thread_exit();

private:
    TaskQueue *taskQ;

    pthread_t manager_id;       //管理者线程ID
    pthread_t *thread_ids;      //工作的线程ID
    int min_num;                //最小线程数量
    int max_num;                //最大线程数量
    int busy_num;               //忙的线程的个数
    int live_num;               //存活的线程个数
    int exit_num;               //要销毁的线程个数
    pthread_mutex_t mutex_pool; //锁整个的线程池
    pthread_cond_t not_empty;
    static const int NUMBER = 2;

    bool shutdown;
};
