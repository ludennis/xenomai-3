#ifndef _DMADIGITALINPUTOUTPUTTASK_H_
#define _DMADIGITALINPUTOUTPUTTASK_H_

#include <DigitalInputOutputTask.h>


#include <streamHelper.h>
#include <CHInCh/dmaErrors.h>
#include <CHInCh/dmaProperties.h>
#include <CHInCh/tCHInChDMAChannel.h>

class DmaDigitalInputOutputTask : public DigitalInputOutputTask
{
protected:
  nNISTC3::tDMAChannelNumber dmaChannel;
  std::unique_ptr<nNISTC3::streamHelper> streamHelper;
public:
  DmaDigitalInputOutputTask();
  void InitStreamHelper();
};

#endif // _DMADIGITALINPUTOUTPUTTASK_H_
