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
using idlMotorOutputMessageType = MotorControllerUnitModule::MotorOutputMessage;
using idlNodejsRequestMessageType = MotorControllerUnitModule::NodejsRequestMessage;

constexpr auto kTaskStackSize = 0;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kHighTaskPriority = 90;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 200000; // 200 us
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;

static dds_bridge::DDSBridge ddsBridge;

static RT_TASK rtTask;
static RT_TASK rtNodejsRequestTask;
static RTIME oneSecondTimer;
static auto numberOfMessagesSent{0u};
static auto numberOfMessagesReceived{0u};

// conditions for waitisets
static dds::core::cond::WaitSet::ConditionSeq conditionSeq;
static dds::core::Duration waitSetConnectionTimeout(5, 0);
static dds::core::Duration waitSetTransmissionTimeout(1, 0);

void WaitForNodejsRequestRoutine(void*)
{
  // subscriber to nodejs_request_topic
  // sends motor a request for MotorOutputMessage
  // then forwards MotorOutputMessage from motor to nodejs

  ddsBridge.CreateDomainParticipant();

  ddsBridge.AddPublisherPartition("ControlCommandPartition");
  ddsBridge.AddPublisherPartition("NodejsPartition");
  ddsBridge.CreatePublisher();

  ddsBridge.AddSubscriberPartition("NodejsPartition");
  ddsBridge.AddSubscriberPartition("MotorResponsePartition");
  ddsBridge.CreateSubscriber();

  auto writerQos = ddsBridge.CreateDataWriterQos();
  ddsBridge.AddQos(writerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));
  ddsBridge.AddQos(writerQos,
    dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances());

  auto motorOutputWriter =
    ddsBridge.CreateDataWriter<idlMotorOutputMessageType>("forwarded_motor_output_topic", writerQos);
  auto controlCommandWriter =
    ddsBridge.CreateDataWriter<idlControlCommandMessageType>("control_command_topic", writerQos);

  auto readerQos = ddsBridge.CreateDataReaderQos();
  ddsBridge.AddQos(readerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));

  auto nodejsRequestReader =
    ddsBridge.CreateDataReader<idlNodejsRequestMessageType>("nodejs_request_topic", readerQos);
  auto motorOutputReader =
    ddsBridge.CreateDataReader<idlMotorOutputMessageType>("motor_output_topic", readerQos);

  ddsBridge.CreateWaitSet();

  auto receivedNodejsRequestStatusCondition =
    ddsBridge.CreateStatusCondition<idlNodejsRequestMessageType>(nodejsRequestReader);
  ddsBridge.EnableStatus(receivedNodejsRequestStatusCondition,
    dds::core::status::StatusMask::data_available());
  auto receivedMotorOutputMessageStatusCondition =
    ddsBridge.CreateStatusCondition<idlMotorOutputMessageType>(motorOutputReader);
  ddsBridge.EnableStatus(receivedMotorOutputMessageStatusCondition,
    dds::core::status::StatusMask::data_available());

  ddsBridge.AddStatusCondition(receivedNodejsRequestStatusCondition);
  ddsBridge.AddStatusCondition(receivedMotorOutputMessageStatusCondition);
  ddsBridge.AddGuardCondition(ddsBridge.mTerminationGuard);

  dds::core::cond::WaitSet::ConditionSeq conditionSeq;

  while(!ddsBridge.mTerminationGuard.trigger_value())
  {
    std::cout << "Waiting for Nodejs Request ..." << std::endl;
    conditionSeq = ddsBridge.mWaitSet.wait();

    for(const auto &condition : conditionSeq)
    {
      if(condition == receivedNodejsRequestStatusCondition)
      {
        dds::sub::LoanedSamples<idlNodejsRequestMessageType> samples = nodejsRequestReader.take();
        for(auto sample = samples.begin(); sample < samples.end(); ++sample)
        {
          if(sample->info().valid())
          {
            std::cout << "Received request from nodejs: " << sample->data().request() << std::endl;
            auto controlCommandMessage =
              idlControlCommandMessageType("RequestMsgMotorOutput");
            controlCommandWriter.write(controlCommandMessage);
            std::cout << "Forwarded RequestMsgMotorOutput to motor" << std::endl;
          }
        }
      }
      else if(condition == receivedMotorOutputMessageStatusCondition)
      {
        dds::sub::LoanedSamples<idlMotorOutputMessageType> samples = motorOutputReader.take();
        for(auto sample = samples.begin(); sample < samples.end(); ++sample)
        {
          if(sample->info().valid())
          {
            std::cout << "Received motor output from motor, rpm: "
              << sample->data().ftRotorRPM() << ", ft_CurrentU: "
              << sample->data().ftCurrentU() << std::endl;

            motorOutputWriter.write(sample->data());
            std::cout << "Forwarded MotorOutput to nodejs" << std::endl;
          }
        }
      }
    }
  }
}

void WriteAndTakeRoutine(void*)
{
  /* DomainParticipant */
  ddsBridge.CreateDomainParticipant();

  /* Publisher */
  ddsBridge.AddPublisherPartition("ControlCommandPartition");
  ddsBridge.CreatePublisher();

  /* Subscriber */
  ddsBridge.AddSubscriberPartition("MotorResponsePartition");
  ddsBridge.AddSubscriberPartition("NodejsPartition");
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

  auto nodejsRequestReader =
    ddsBridge.CreateDataReader<idlNodejsRequestMessageType>("nodejs_request_topic", readerQos);

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

  int e1 = rt_task_create(&rtTask, "WriteAndTakeRoutine",
    kTaskStackSize, kMediumTaskPriority, kTaskMode);
  int e2 = rt_task_set_periodic(&rtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
  int e3 = rt_task_start(&rtTask, &WriteAndTakeRoutine, NULL);

  if(e1 | e2 | e3)
  {
    std::cerr << "Error launching periodic WriteAndTakeRoutine. Extiting." << std::endl;
    return -1;
  }

  int e4 = rt_task_create(&rtNodejsRequestTask, "NodejsRequestRoutine",
    kTaskStackSize, kHighTaskPriority, kTaskMode);
  int e5 = rt_task_start(&rtNodejsRequestTask, &WaitForNodejsRequestRoutine, NULL);

  if(e4 | e5)
  {
    std::cerr << "Error launching task NodejsRequestRoutine. Exiting." << std::endl;
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
