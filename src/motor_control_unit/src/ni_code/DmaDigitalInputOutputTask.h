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
  nNISTC3::tDMAChannelNumber dmaChannel;
  std::unique_ptr<nNISTC3::tCHInChDMAChannel> dma;
  std::unique_ptr<nNISTC3::streamHelper> streamHelper;

  nInTimer::tInTimer_Error_t scanOverrun;
  nInTimer::tInTimer_Error_t fifoOverflow;

  unsigned int dmaSizeInBytes;
  unsigned int sampleSizeInBytes;
  unsigned int readSizeInBytes;
  unsigned int bytesAvailable;
  unsigned int bytesRead;

  std::vector<unsigned char> rawData;
public:
  DmaDigitalInputOutputTask();
  void DmaRead();
  void EnableStreamHelper();
  void StartDmaChannel();
  ~DmaDigitalInputOutputTask();
};

#endif // _DMADIGITALINPUTOUTPUTTASK_H_
