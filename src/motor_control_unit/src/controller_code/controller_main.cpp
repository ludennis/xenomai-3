#include <sys/mman.h>

#include <fstream>
#include <thread>
#include <sstream>
#include <signal.h>

#include <alchemy/task.h>

#include <dds/dds.hpp>

#include <dds_code/dds_bridge.hpp>
#include <utils/FileWriter.hpp>
#include <utils/SynchronizedFile.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

using idlControlCommandMessageType = MotorControllerUnitModule::ControlCommandMessage;
using idlMotorResponseMessageType = MotorControllerUnitModule::MotorResponseMessage;

constexpr auto kTaskStackSize = 0;
constexpr auto kTaskPriority = 20;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 120000; // 100 us
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;

static dds_bridge::DDSBridge ddsBridge;

static RT_TASK rtTask;
static RTIME oneSecondTimer;
static auto numberOfMessagesSent{0u};
static auto numberOfMessagesReceived{0u};

// conditions for waitisets
static dds::core::cond::WaitSet::ConditionSeq conditionSeq;
static dds::core::Duration waitSetConnectionTimeout(5, 0);
static dds::core::Duration waitSetTransmissionTimeout(1, 0);

void WriteAndTakeRoutine(void*)
{
  /* DomainParticipant */
  ddsBridge.CreateDomainParticipant();

  /* Publisher */
  ddsBridge.AddPublisherPartition("ControlCommandPartition");
  ddsBridge.CreatePublisher();

  /* Subscriber */
  ddsBridge.AddSubscriberPartition("MotorResponsePartition");
  ddsBridge.CreateSubscriber();

  /* DataWriter */
  auto writerQos = ddsBridge.CreateDataWriterQos();
  ddsBridge.AddQos(writerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));
  ddsBridge.AddQos(writerQos,
    dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances());

  auto controlDataWriter =
    ddsBridge.CreateDataWriter<idlControlCommandMessageType>("control_command_topic", writerQos);

  /* DataReader */
  auto readerQos = ddsBridge.CreateDataReaderQos();
  ddsBridge.AddQos(readerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));

  auto motorDataReader =
    ddsBridge.CreateDataReader<idlMotorResponseMessageType>("motor_response_topic", readerQos);

  /* WaitSet */
  ddsBridge.CreateWaitSet();

  auto motorStatusCondition =
    ddsBridge.CreateStatusCondition<idlMotorResponseMessageType>(motorDataReader);
  ddsBridge.EnableStatus(motorStatusCondition, dds::core::status::StatusMask::data_available());

  ddsBridge.AddStatusCondition(motorStatusCondition);
  ddsBridge.AddGuardCondition(ddsBridge.mTerminationGuard);

  RTIME now, previous;

  previous = rt_timer_read();
  while(!ddsBridge.mTerminationGuard.trigger_value())
  {
    auto controlMessage = idlControlCommandMessageType("motor_step");

    numberOfMessagesSent++;

    dds::core::InstanceHandle instanceHandle =
      controlDataWriter.register_instance(controlMessage);
    controlDataWriter << controlMessage;
    controlDataWriter.unregister_instance(instanceHandle);

    rt_task_wait_period(NULL);

    auto samples = motorDataReader.take();

    if(samples.begin() != samples.end())
    {
      numberOfMessagesReceived++;
    }
    else
    {
      std::cout << "take() results in empty sample" << std::endl;
    }

    now = rt_timer_read();

    if(static_cast<long>(now - oneSecondTimer) / kNanosecondsToSeconds > 0)
    {
      std::cout << "number of messages sent / received / packet rate loss: " <<
        numberOfMessagesSent << " / " << numberOfMessagesReceived << " / " <<
        static_cast<double>(numberOfMessagesSent - numberOfMessagesReceived) /
          numberOfMessagesSent * 100. <<
        "%" << std::endl;
      std::cout << "Time elapsed for task: " <<
        static_cast<long>(now - previous) / kNanosecondsToMicroseconds <<
        "." << static_cast<long>(now - previous) % kNanosecondsToMicroseconds <<
        " us" << std::endl;

      oneSecondTimer = now;
    }

    previous = now;
  }
}

void TerminationHandler(int s)
{
  std::cout << "Caught Termination Signal with code: " << s << std::endl;
  ddsBridge.mTerminationGuard.trigger_value(true);
}

int main(int argc, char *argv[])
{
  std::string outputFilename;
  auto outputFile = std::make_shared<utils::SynchronizedFile>();

  if(argc == 0)
  {
    std::cerr << "Usage: controller [latency output]" << std::endl;
    return -1;
  }

  if(argc >= 2)
  {
    outputFilename = argv[1];
  }

  if(!outputFilename.empty())
  {
    std::cout << "Writing to " << outputFilename << " ..." << std::endl;
    outputFile->open(outputFilename);
    std::stringstream header;
    header << "write,roundtrip,take" << std::endl;
    auto fileWriter = utils::FileWriter(outputFile, header.str());
  }

  // signal handler for ctrl + c
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  std::cout << "Connecting to motor ...";

  mlockall(MCL_CURRENT | MCL_FUTURE);

  oneSecondTimer = rt_timer_read();

  int e1 = rt_task_create(&rtTask, "WriteAndTakeRoutine", kTaskStackSize, kTaskPriority, kTaskMode);
  int e2 = rt_task_set_periodic(&rtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
  int e3 = rt_task_start(&rtTask, &WriteAndTakeRoutine, NULL);

  if(e1 | e2 | e3)
  {
    std::cerr << "Error launching periodic task. Extiting." << std::endl;
    return -1;
  }

  while(!ddsBridge.mTerminationGuard.trigger_value())
  {}

  if(!outputFilename.empty())
  {
    outputFile->close();
  }

  std::cout << "Controller Exiting ..." << std::endl;

  return 0;
}
