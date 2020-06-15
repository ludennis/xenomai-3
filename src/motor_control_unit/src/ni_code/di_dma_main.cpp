#include <DmaDigitalInputOutputTask.h>

int main(int argc, char *argv[])
{
  auto dmaDiTask = std::make_unique<DmaDigitalInputOutputTask>();
  dmaDiTask->Init(argv[1], argv[2]);
  dmaDiTask->StartDmaChannel();
  dmaDiTask->EnableStreamHelper();
  dmaDiTask->ProgramDiSubsystem(10000u, 2u);
  dmaDiTask->ArmAndStartDiSubsystem();
  dmaDiTask->DmaRead();
  dmaDiTask->Stop();

  return 0;
}
