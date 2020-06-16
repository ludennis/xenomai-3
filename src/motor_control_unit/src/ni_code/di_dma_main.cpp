#include <signal.h>
#include <chrono>
#include <thread>

#include <DmaDigitalInputOutputTask.h>

std::unique_ptr<DmaDigitalInputOutputTask> dmaDiTask;

void terminationHandler(int signal)
{
  dmaDiTask->Stop();
  dmaDiTask.reset();
  exit(1);
}

int main(int argc, char *argv[])
{
  struct sigaction action;
  action.sa_handler = terminationHandler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT , &action, NULL);

  dmaDiTask = std::make_unique<DmaDigitalInputOutputTask>();
  dmaDiTask->Init(argv[1], argv[2]);
  dmaDiTask->StartDmaChannel();
  dmaDiTask->EnableStreamHelper();
  dmaDiTask->ProgramDiSubsystem(10000u, 2u);
  dmaDiTask->ArmAndStartDiSubsystem();

  for (;;)
  {
    dmaDiTask->DmaRead();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
//  dmaDiTask->Stop();

  return 0;
}
