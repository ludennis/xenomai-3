#include <DmaDigitalInputOutputTask.h>

#include <osiBus.h>
#include <tXSeries.h>

#include <dmaErrors.h>
#include <dmaProperties.h>
#include <tCHInChDMAChannel.h>

int main(int argc, char *argv[])
{
  auto dmaDiTask = std::make_unique<DmaDigitalInputOutputTask>();
  dmaDiTask->Init(argv[1], argv[2]);

  return 0;
}
