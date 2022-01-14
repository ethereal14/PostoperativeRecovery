#pragma once
#include <queue>
#include "pthread.h"

using callback = void (*)(void *arg);
struct Task
{
    Task() : function(nullptr), arg(nullptr)
    {
    }
    Task(callback f, void *arg)
    {
        this->function = f;
        this->arg = arg;
    }
    callback function;
    void *arg;
};

class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    void addTask(Task &task);
    void addTask(callback f, void *arg);
    Task getTask();
    inline int getNumber() { return _taskQ.size(); }

private:
    std::queue<Task> _taskQ;
    pthread_mutex_t _mutex;
};