#ifndef _DMADIGITALINPUTOUTPUTTASK_H_
#define _DMADIGITALINPUTOUTPUTTASK_H_

#include <vector>

#include <DigitalInputOutputTask.h>

#include <streamHelper.h>
#include <CHInCh/dmaErrors.h>
#include <CHInCh/dmaProperties.h>
#include <CHInCh/tCHInChDMAChannel.h>

class DmaDigitalInputOutputTask : public DigitalInputOutputTask
{
protected:
  nMDBG::tStatus2 dmaError;
  tBoolean hasDmaError;
  tBoolean allowOverwrite;
  tBoolean hasDataOverwritten;
  tBoolean hasDiError;
  tBoolean dataOverwritten;

  nNISTC3::tDMAChannelNumber dmaChannel;
  nNISTC3::inTimerParams timingConfig;

  std::unique_ptr<nNISTC3::tCHInChDMAChannel> dma;
  std::unique_ptr<nNISTC3::streamHelper> streamHelper;

  nInTimer::tInTimer_Error_t scanOverrun;
  nInTimer::tInTimer_Error_t fifoOverflow;

  nDI::tDI_DataWidth_t dataWidth;
  nDI::tDI_Data_Lane_t dataLane;

  unsigned int dmaSizeInBytes;
  unsigned int sampleSizeInBytes;
  unsigned int readSizeInBytes;
  unsigned int bytesAvailable;
  unsigned int bytesRead;
  unsigned int samplesPerChannel;

  std::vector<unsigned char> rawData;

  double runTime;
public:
  DmaDigitalInputOutputTask();
  void ArmAndStartDiSubsystem();
  void DmaRead();
  void EnableStreamHelper();
  void ProgramDiSubsystem(const unsigned int samplePeriod, const unsigned int sampleDelay);
  void StartDmaChannel();
  void Stop();
  tBoolean HasDiError();
  tBoolean HasDmaError();
  ~DmaDigitalInputOutputTask();
};

#endif // _DMADIGITALINPUTOUTPUTTASK_H_
