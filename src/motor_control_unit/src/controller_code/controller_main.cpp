#include <sys/mman.h>

#include <fstream>
#include <thread>
#include <sstream>
#include <signal.h>

#include <alchemy/task.h>

#include <dds/dds.hpp>

#include <dds_code/Entities.h>
#include <utils/FileWriter.hpp>
#include <utils/SynchronizedFile.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

constexpr auto kTaskStackSize = 0;
constexpr auto kTaskPriority = 20;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 120000; // 100 us
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;

static dds_entities::Entities entities("controller", "motor");
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
  RTIME now, previous;

  previous = rt_timer_read();
  while(!entities.mTerminationGuard.trigger_value())
  {
    auto controlMessage = MotorControllerUnitModule::ControlMessage("motor_step");

    numberOfMessagesSent++;

    dds::core::InstanceHandle instanceHandle =
      entities.mControlMessageWriter.register_instance(controlMessage);
    entities.mControlMessageWriter << controlMessage;
    entities.mControlMessageWriter.unregister_instance(instanceHandle);

    rt_task_wait_period(NULL);

    auto samples = entities.mMotorMessageReader.take();

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
  entities.mTerminationGuard.trigger_value(true);
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
  for(;;)
  {
    entities.mControlMessageWriter <<
      MotorControllerUnitModule::ControlMessage("connect");
    try
    {
      entities.mMotorMessageWaitSet.wait(conditionSeq, waitSetConnectionTimeout);
      entities.mMotorMessageReader.take();
      std::cout << "Connected! Warming Up ..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(5));
      break;
    }
    catch(dds::core::TimeoutError &e)
    {
      // TODO: check if motor is off, if it's off, exit.
      (void)e;
    }
  }

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

  while(!entities.mTerminationGuard.trigger_value())
  {}

  if(!outputFilename.empty())
  {
    outputFile->close();
  }

  std::cout << "Controller Exiting ..." << std::endl;

  return 0;
}
