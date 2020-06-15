#include <DmaDigitalInputOutputTask.h>

DmaDigitalInputOutputTask::DmaDigitalInputOutputTask()
{}

void DmaDigitalInputOutputTask::InitStreamHelper()
{
  streamHelper = std::make_unique<nNISTC3::streamHelper>(
    device->DIStreamCircuit, device->CHInCh, status);
}
