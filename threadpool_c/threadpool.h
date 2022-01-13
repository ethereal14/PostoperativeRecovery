#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "pthread.h"

typedef struct task
{
    void (*function)(void *arg);
    void *arg;
} task;

struct threadpool
{
    task *taskQ;
    int queue_capacity; //容量
    int queue_size;     //当前任务个数
    int queue_front;    //队头 -> 取数据
    int queue_rear;     //队尾 -> 放数据

    pthread_t manager_id;       //管理者线程ID
    pthread_t *thread_ids;      //工作的线程ID
    int min_num;                //最小线程数量
    int max_num;                //最大线程数量
    int busy_num;               //忙的线程的个数
    int live_num;               //存活的线程个数
    int exit_num;               //要销毁的线程个数
    pthread_mutex_t mutex_pool; //锁整个的线程池
    pthread_mutex_t mutex_busy; //锁busyNum变量
    pthread_cond_t not_full;    //任务队列是不是满了
    pthread_cond_t not_empty;   //任务队列是不是空了

    int shutdown;
};

typedef struct threadpool threadpool;

/**
 * @brief 创建线程池并初始化
 *
 * @param min 最少工作线程
 * @param max 最大工作线程
 * @param queue_size 当前队列大小
 * @return threadpool*
 */
threadpool *threadpool_create(int min, int max, int queue_capacity);

/**
 * @brief 销毁线程池
 *
 * @param pool 线程池指针
 * @return int
 */
int threadpool_destroy(threadpool *pool);

/**
 * @brief 向线程池添加任务
 *
 * @param pool 线程池指针
 * @param func 任务函数
 * @param arg 人物函数参数
 */
void threadpool_add(threadpool *pool, void (*func)(void *), void *arg);

/**
 * @brief 获取线程池中工作的线程个数
 *
 * @param pool 线程池指针
 * @return int
 */
int threadpool_busy_num(threadpool *pool);

/**
 * @brief 获取线程池中活着的线程的个数
 *
 * @param pool 线程池指针
 * @return int
 */
int threadpool_live_num(threadpool *pool);

/**
 * @brief 工作的线程任务函数
 *
 * @param arg
 * @return void*
 */
void *worker(void *arg);

/**
 * @brief 管理者线程任务函数
 *
 * @param arg
 * @return void*
 */
void *manager(void *arg);

/**
 * @brief 单个线程退出
 *
 * @param pool
 */
void thread_exit(threadpool *pool);
#endif // THREADPOOL_H