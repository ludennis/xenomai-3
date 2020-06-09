#include <alchemy/task.h>

#include <RtPeriodicTask.h>

#include <DigitalInputOutputTask.h>

// TODO: make this a RT_xenomai periodic Task
int main(int argc, char* argv[])
{
  if (argc <= 2)
  {
    printf("Usage: do_main [PCI Bus Number] [PCI Device Number]\n");
    return -1;
  }

  DigitalInputOutputTask dioTask;
  dioTask.Init(argv[1], argv[2]);
  dioTask.Write();

  return 0;
}
