#include <iostream>

#include <alchemy/task.h>
#include <sys/mman.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kTaskPriority = 20;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 100000; // 100 us

RT_TASK rtTask;

void periodicTask(void *arg)
{
  RTIME now, previous;

  previous = rt_timer_read();
  for(;;)
  {
    rt_task_wait_period(NULL);
    now = rt_timer_read();

    std::cout << "Time elapsed: " << static_cast<long>(now - previous) / 1000 <<
      "." << static_cast<long>(now - previous) % 1000 << " us" << std::endl;

    previous = now;
  }
}

int main(int argc, char* argv[])
{
  std::cout << "test_rt runned" << std::endl;

  mlockall(MCL_CURRENT|MCL_FUTURE);

  int e1 = rt_task_create(&rtTask, "periodTask", kTaskStackSize, kTaskPriority, kTaskMode);
  int e2 = rt_task_set_periodic(&rtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
  int e3 = rt_task_start(&rtTask, &periodicTask, NULL);

  if(e1 | e2 | e3)
  {
    rt_task_delete(&rtTask);
    std::cerr << "Error launching periodic task. Extiting." << std::endl;
    exit(1);
  }

  std::cout << "Press any key to continue ..." << std::endl;
  std::string input;
  std::cin >> input;

  return 0;
}
