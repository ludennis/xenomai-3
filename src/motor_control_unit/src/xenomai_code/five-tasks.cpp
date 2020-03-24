#include <chrono>
#include <stdio.h>
#include <thread>

#include <alchemy/task.h>

auto constexpr kNumberOfTasks = 5;

RT_TASK tasks[kNumberOfTasks];

RTIME oneSecond = 1e9;
RTIME twoSeconds = 2e9;
RTIME threeSeconds = 3e9;

void demo(void *arg)
{
  RT_TASK_INFO currentTaskInfo;
  rt_task_inquire(NULL, &currentTaskInfo);
  rt_task_sleep(oneSecond);

  for(;;)
  {
    int index = * (int *) arg;
    rt_printf("Task name: %s, task index: %i \n", currentTaskInfo.name, index);
    rt_task_wait_period(NULL);
  }

  return;
}

int main(int argc, char** argv)
{
  rt_task_create(&tasks[0], "Task 0", 0, 99, 0);
  rt_task_create(&tasks[1], "Task 1", 0, 99, 0);
  rt_task_create(&tasks[2], "Task 2", 0, 99, 0);

  rt_task_set_periodic(&tasks[0], TM_NOW, oneSecond);
  rt_task_set_periodic(&tasks[1], TM_NOW, twoSeconds);
  rt_task_set_periodic(&tasks[2], TM_NOW, threeSeconds);

  for(auto i{0}; i < kNumberOfTasks; ++i)
  {
    rt_task_start(&tasks[i], &demo, &i);
  }

  for(;;)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
