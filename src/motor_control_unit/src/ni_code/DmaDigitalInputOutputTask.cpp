#include <DmaDigitalInputOutputTask.h>

DmaDigitalInputOutputTask::DmaDigitalInputOutputTask()
{}

void DmaDigitalInputOutputTask::DmaRead()
{
  dma->read(0, NULL, &bytesAvailable, allowOverwrite, &hasDataOverwritten, status);
  if (status.isFatal())
  {
    dmaError = status;
    hasDmaError = kTrue;
    return;
  }
  else if (bytesAvailable >= readSizeInBytes)
  {
    dma->read(readSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite,
      &hasDataOverwritten, status);
    if (status.isNotFatal())
    {
      nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);
      bytesRead += readSizeInBytes;

      if (!allowOverwrite)
      {
        streamHelper->modifyTransferSize(readSizeInBytes, status);
      }
    }
    else
    {
      dmaError = status;
      hasDmaError = kTrue;
    }
  }

  device->DI.DI_Timer.Status_1_Register.refresh(&status);
  scanOverrun = device->DI.DI_Timer.Status_1_Register.getOverrun_St(&status);
  fifoOverflow = device->DI.DI_Timer.Status_1_Register.getOverflow_St(&status);

  if (scanOverrun || fifoOverflow)
  {
    hasDiError = kTrue;
  }

//  if (hasDiError || hasDmaError)
//  {
//    break;
//  }
}

void DmaDigitalInputOutputTask::EnableStreamHelper()
{
  if (dma == nullptr)
  {
    printf("ERROR DmaDigitalInputOutputTask::StartStreamHelper: dma is nullptr");
    return;
  }

  streamHelper = std::make_unique<nNISTC3::streamHelper>(
    device->DIStreamCircuit, device->CHInCh, status);

  streamHelper->configureForInput(!allowOverwrite, dmaChannel, status);
  if (!allowOverwrite)
  {
    streamHelper->modifyTransferSize(dmaSizeInBytes, status);
  }
  streamHelper->enable(status);
}

void DmaDigitalInputOutputTask::StartDmaChannel()
{
  dmaChannel = nNISTC3::kDI_DMAChannel;

  dma = std::make_unique<nNISTC3::tCHInChDMAChannel>(*device, dmaChannel, status);

  if (status.isFatal())
  {
    printf("Error: DMA channel initialization (%d).\n", status);
    return;
  }

  dma->reset(status);

  sampleSizeInBytes = (port0Length == 32) ? 4 : 1;
  const unsigned int samplesPerChannel = 1024;
  const unsigned int dmaBufferFactor = 16;
  readSizeInBytes = samplesPerChannel * sampleSizeInBytes;
  dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;

  dma->configure(bus, nNISTC3::kReuseLinkRing, nNISTC3::kIn, dmaSizeInBytes, status);
  if (status.isFatal())
  {
    printf("Error: DMA channel configuration (%d).\n", status);
    return;
  }

  dma->start(status);
  if (status.isFatal())
  {
    printf("Error: DMA channel start (%d).\n", status);
    return;
  }
}

DmaDigitalInputOutputTask::~DmaDigitalInputOutputTask()
{
  streamHelper->disable(status);
  dma->stop(status);
}
