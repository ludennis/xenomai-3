#ifndef _RTPEAKCANTRANSMITTASK_H_
#define _RTPEAKCANTRANSMITTASK_H_

#include <stdlib.h>
#include <sys/mman.h>

#include <PeakCanTransmitTask.h>
#include <RtPeriodicTask.h>

class RtPeakCanTransmitTask : public PeakCanTransmitTask, public RtPeriodicTask
{
public:
  RtPeakCanTransmitTask() = delete;
  RtPeakCanTransmitTask(const char *deviceName, const unsigned int baudRate,
    const char* name, const int stackSize, const int priority, const int mode,
    const int period, const int coreId);

  int StartRoutine();
  static void Routine(void*);

  ~RtPeakCanTransmitTask();
};

#endif // _RTPEAKCANTRANSMITTASK_H_
