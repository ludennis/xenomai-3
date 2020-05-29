#ifndef _PEAKCANTRANSMITTASK_H_
#define _PEAKCANTRANSMITTASK_H_

#include <PeakCanTask.h>

class PeakCanTransmitTask : public PeakCanTask
{
public:
  PeakCanTransmitTask() = delete;
  PeakCanTransmitTask(const char *deviceName, const unsigned int baudRate);

  int Write(TPCANMsg &writeMessage);
};

#endif // _PEAKCANTRANSMITTASK_H_
