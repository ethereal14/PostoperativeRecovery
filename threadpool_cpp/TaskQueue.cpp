#include "TaskQueue.h"

TaskQueue::TaskQueue()
{
    pthread_mutex_init(&_mutex, NULL);
}

TaskQueue::~TaskQueue()
{
    pthread_mutex_destroy(&_mutex);
}

void TaskQueue::addTask(Task &task)
{
    pthread_mutex_lock(&_mutex);
    _taskQ.push(task);
    pthread_mutex_unlock(&_mutex);
}

void TaskQueue::addTask(callback f, void *arg)
{
    pthread_mutex_lock(&_mutex);
    _taskQ.push(Task(f, arg));
    pthread_mutex_unlock(&_mutex);
}

Task TaskQueue::getTask()
{
    Task t;
    pthread_mutex_lock(&_mutex);

    if (!_taskQ.empty())
    {
        t = _taskQ.front();
        _taskQ.pop();
    }
    pthread_mutex_unlock(&_mutex);
    return t;
}