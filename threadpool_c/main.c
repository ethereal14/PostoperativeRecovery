#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include "unistd.h"

void taskFunc(void *arg)
{
    int num = *(int *)arg;
    printf("thread %ld is working,number = %d\n",
           pthread_self(), num);
    sleep(1);
}

int main(int argc, char **argv)
{
    threadpool *pool = threadpool_create(3, 10, 100);
    for (int i = 0; i < 100; i++)
    {
        int *num = (int *)malloc(sizeof(int));
        *num = i + 100;
        threadpool_add(pool, taskFunc, num);
    }
    sleep(5);

    threadpool_destroy(pool);
    return 0;
}