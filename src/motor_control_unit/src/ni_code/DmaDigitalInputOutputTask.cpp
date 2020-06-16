#include <DmaDigitalInputOutputTask.h>

nMDBG::tStatus2 DmaDigitalInputOutputTask::dmaError;
tBoolean DmaDigitalInputOutputTask::hasDiError;
tBoolean DmaDigitalInputOutputTask::hasDmaError;
tBoolean DmaDigitalInputOutputTask::allowOverwrite;
tBoolean DmaDigitalInputOutputTask::hasDataOverwritten;
nInTimer::tInTimer_Error_t DmaDigitalInputOutputTask::scanOverrun;
nInTimer::tInTimer_Error_t DmaDigitalInputOutputTask::fifoOverflow;
unsigned int DmaDigitalInputOutputTask::readSizeInBytes;
unsigned int DmaDigitalInputOutputTask::sampleSizeInBytes;
unsigned int DmaDigitalInputOutputTask::bytesAvailable;
unsigned int DmaDigitalInputOutputTask::bytesRead;
std::unique_ptr<nNISTC3::streamHelper> DmaDigitalInputOutputTask::streamHelper;
std::unique_ptr<nNISTC3::tCHInChDMAChannel> DmaDigitalInputOutputTask::dma;
std::vector<unsigned char> DmaDigitalInputOutputTask::rawData;


DmaDigitalInputOutputTask::DmaDigitalInputOutputTask(
  const char * name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

void DmaDigitalInputOutputTask::ArmAndStartDiSubsystem()
{
  runTime = 10;
  diHelper->getInTimerHelper(status).armTiming(timingConfig, status);
  printf("Starting continuous %.2f-second harware-timed digital measurement.\n", runTime);
  printf("Reading %u-sample chunks from the %u-sample DMA buffer.\n",
    samplesPerChannel, dmaSizeInBytes/sampleSizeInBytes);
  diHelper->getInTimerHelper(status).strobeStart1(status);
  nNISTC3::nDIODataHelper::printHeader(0);
}

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

void DmaDigitalInputOutputTask::ProgramDiSubsystem(
  const unsigned int samplePeriod, const unsigned int sampleDelay)
{
  dataWidth = (port0Length == 32) ? nDI::kDI_FourBytes : nDI::kDI_OneByte;
  dataLane = nDI::kDI_DataLane0;

  diHelper->reset(status);
  dioHelper->reset(NULL, 0, status);

  device->DI.DI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

  diHelper->programExternalGate(nDI::kGate_Disabled, nDI::kActive_High_Or_Rising_Edge, status);
  diHelper->programStart1(nDI::kStart1_SW_Pulse, nDI::kActive_High_Or_Rising_Edge, kTrue, status);
  diHelper->programConvert(nDI::kSampleClk_Internal, nDI::kActive_High_Or_Rising_Edge, status);

  timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerContinuous, status);
  timingConfig.setUseSICounter(kTrue, status);
  timingConfig.setSamplePeriod(samplePeriod, status);
  timingConfig.setSampleDelay(sampleDelay, status);
  timingConfig.setNumberOfSamples(0, status);

  diHelper->getInTimerHelper(status).programTiming(timingConfig, status);

  device->DI.DI_Mode_Register.setDI_DataWidth(dataWidth, &status);
  device->DI.DI_Mode_Register.setDI_Data_Lane(dataLane, &status);
  device->DI.DI_Mode_Register.flush(&status);

  diHelper->getInTimerHelper(status).clearFIFO(status);

  dioHelper->configureLines(lineMaskPort0, nNISTC3::kCorrInput, status);
  device->DI.DI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);
}

void DmaDigitalInputOutputTask::Routine(void*)
{
  for (;;)
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

    rt_task_wait_period(NULL);
  }
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
  samplesPerChannel = 1024;
  const unsigned int dmaBufferFactor = 16;
  readSizeInBytes = samplesPerChannel * sampleSizeInBytes;
  dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;
  rawData.assign(readSizeInBytes, 0);

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

void DmaDigitalInputOutputTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_set_affinity(&mRtTask, &mCpuSet);
  int e4 = rt_task_start(&mRtTask, Routine, NULL);

  if (e1 | e2 | e3 | e4)
  {
    printf("DmaDigitalInputOutputTask::StartRoutine error for rt_task\n");
    exit(-1);
  }
}

void DmaDigitalInputOutputTask::Stop()
{
  double rlpElapsedTime;
  clock_t rlpStart;
  const double rlpTimeout = 5;

  if (!hasDiError)
  {
    device->DI.DI_Timer.Command_Register.writeEnd_On_End_Of_Scan(kTrue, &status);

    rlpElapsedTime = 0;
    rlpStart = clock();
    while (device->DI.DI_Timer.Status_1_Register.readSC_Armed_St(&status))
    {
      if (rlpElapsedTime > rlpTimeout)
      {
        printf("\n");
        printf("Error: DI timing engine did not stop within timeout.\n");
        status.setCode(kStatusRLPTimeout);
        return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
    }

    rlpElapsedTime = 0;
    rlpStart = clock();
    while (!streamHelper->fifoIsEmpty(status))
    {
      if (rlpElapsedTime > rlpTimeout)
      {
        printf("\n");
        printf("Error: Stream circuit did not flush within timeout.\n");
        status.setCode(kStatusRLPTimeout);
        return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
    }
  }

  dma->read(0, NULL, &bytesAvailable, allowOverwrite, &dataOverwritten, status);
  if (status.isFatal())
  {
    dmaError = status;
    hasDmaError = kTrue;
  }

  while (bytesAvailable)
  {
    if (bytesAvailable < readSizeInBytes)
    {
      readSizeInBytes = bytesAvailable;
    }
    dma->read(readSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite,
      &dataOverwritten, status);
    if (status.isNotFatal())
    {
      nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);
      bytesRead += readSizeInBytes;
    }
    else
    {
      dmaError = status;
      hasDmaError = kTrue;
      break;
    }
  }

  if (hasDmaError)
  {
    printf("Error: DMA read (%d).\n", dmaError);
    status.setCode(kStatusRuntimeError);
  }
  if (scanOverrun)
  {
    printf("Error: DI sample clock overrun.\n");
    status.setCode(kStatusRuntimeError);
  }
  if (fifoOverflow)
  {
    printf("Error: DI FIFO overflow.\n");
    status.setCode(kStatusRuntimeError);
  }
  if (!(hasDiError || hasDmaError))
  {
    printf("Finished continuous %.2f-second hardware-timed digital measurement.\n", runTime);
    printf("Read %u samples (%s) using a %u-sample DMA buffer.\n",
      bytesRead/sampleSizeInBytes,
      dataOverwritten ? "by overwriting data" : "without overwriting data",
      dmaSizeInBytes/sampleSizeInBytes);
  }
}

tBoolean DmaDigitalInputOutputTask::HasDiError()
{
  return hasDiError;
}

tBoolean DmaDigitalInputOutputTask::HasDmaError()
{
  return hasDmaError;
}

DmaDigitalInputOutputTask::~DmaDigitalInputOutputTask()
{
  streamHelper->disable(status);
  dma->stop(status);
}
