#include <stdio.h>

#include <alchemy/task.h>
#include <alchemy/sem.h>

#define ITER 10

RTIME oneSecond = 1e9;

static RT_TASK t1;
static RT_TASK t2;

static RT_SEM semaphore;

int global = 0;

void taskOne(void *arg)
{
  for(int i = 0; i < ITER; ++i)
  {
    rt_sem_p(&semaphore, oneSecond);
    printf("I am taskOne and global = %d ... \n", ++global);
    rt_sem_v(&semaphore);
    rt_sem_broadcast(&semaphore);
  }
}

void taskTwo(void *arg)
{
  for(int i = 0; i < ITER; ++i)
  {
    rt_sem_p(&semaphore, oneSecond);
    printf("I am taskTwo and global = %d ... \n", --global);
    rt_sem_v(&semaphore);
    rt_sem_broadcast(&semaphore);
  }
}

int main(int argc, char** argv)
{
  rt_sem_create(&semaphore, "semaphore", 1, 0);

  rt_task_create(&t1, "task1", 0, 1, 0);
  rt_task_create(&t2, "task2", 0, 1, 0);
  rt_task_start(&t1, &taskOne, 0);
  rt_task_start(&t2, &taskTwo, 0);
  return 0;
}
