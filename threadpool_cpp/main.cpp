#include "ThreadPool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void taskFunc(void *arg)
{
    int num = *(int *)arg;
    printf("thread %ld is working,number = %d\n",
           pthread_self(), num);
    sleep(1);
}

int main(int argc, char **argv)
{
    ThreadPool pool(3, 10);
    for (int i = 0; i < 100; i++)
    {
        int *num = new int(i + 100);
        pool.addTask(Task(taskFunc, num));
    }
    sleep(5);

    return 0;
}