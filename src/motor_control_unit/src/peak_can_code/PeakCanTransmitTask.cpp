#include <PeakCanTransmitTask.h>

PeakCanTransmitTask::PeakCanTransmitTask(
  const char *deviceName, const unsigned int baudRate)
  : PeakCanTask(deviceName, baudRate)
{}

int PeakCanTransmitTask::Write(TPCANMsg &writeMessage)
{
  CAN_Write(mHandle, &writeMessage);
}
