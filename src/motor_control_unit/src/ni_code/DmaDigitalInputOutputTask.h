#ifndef _DMADIGITALINPUTOUTPUTTASK_H_
#define _DMADIGITALINPUTOUTPUTTASK_H_

#include <vector>

#include <DigitalInputOutputTask.h>
#include <RtPeriodicTask.h>

#include <streamHelper.h>
#include <CHInCh/dmaErrors.h>
#include <CHInCh/dmaProperties.h>
#include <CHInCh/tCHInChDMAChannel.h>

class DmaDigitalInputOutputTask : public DigitalInputOutputTask, public RtPeriodicTask
{
protected:
  static nMDBG::tStatus2 dmaError;
  static tBoolean hasDmaError;
  static tBoolean allowOverwrite;
  static tBoolean hasDataOverwritten;
  static tBoolean hasDiError;
  tBoolean dataOverwritten;

  nNISTC3::tDMAChannelNumber dmaChannel;
  nNISTC3::inTimerParams timingConfig;

  static std::unique_ptr<nNISTC3::tCHInChDMAChannel> dma;
  static std::unique_ptr<nNISTC3::streamHelper> streamHelper;

  static nInTimer::tInTimer_Error_t scanOverrun;
  static nInTimer::tInTimer_Error_t fifoOverflow;

  nDI::tDI_DataWidth_t dataWidth;
  nDI::tDI_Data_Lane_t dataLane;

  unsigned int dmaSizeInBytes;
  static unsigned int sampleSizeInBytes;
  static unsigned int readSizeInBytes;
  static unsigned int bytesAvailable;
  static unsigned int bytesRead;
  unsigned int samplesPerChannel;

  static std::vector<unsigned char> rawData;

  double runTime;
public:
  DmaDigitalInputOutputTask() = delete;
  DmaDigitalInputOutputTask(const char *name, const int stackSize, const int priority,
    const int mode, const int period, const int coreId);
  void ArmAndStartDiSubsystem();
  void DmaRead();
  void EnableStreamHelper();
  void ProgramDiSubsystem(const unsigned int samplePeriod, const unsigned int sampleDelay);
  static void Routine(void*);
  void StartDmaChannel();
  void StartRoutine();
  void Stop();
  tBoolean HasDiError();
  tBoolean HasDmaError();
  ~DmaDigitalInputOutputTask();
};

#endif // _DMADIGITALINPUTOUTPUTTASK_H_
