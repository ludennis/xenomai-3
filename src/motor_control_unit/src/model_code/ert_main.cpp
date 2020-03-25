#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <sstream>

#include <dds/dds.hpp>

#include <dds_code/dds_bridge.hpp>
#include "generated_model.h"
#include "input_interface.h"
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

using idlControlCommandMessageType = MotorControllerUnitModule::ControlCommandMessage;
using idlMotorResponseMessageType = MotorControllerUnitModule::MotorResponseMessage;
using idlMotorOutputMessageType = MotorControllerUnitModule::MotorOutputMessage;

static dds_bridge::DDSBridge ddsBridge;

namespace {

void motorStep()
{
  generated_model_step();
}

void OutputMsgMotorOutput(const MsgMotorOutput& output)
{
  std::cout << "MsgMotorOutput: ft_CurrentU = " << output.ft_CurrentU
    << ", ft_CurrentV = " << output.ft_CurrentV << ", ft_CurrentW = " << output.ft_CurrentW
    << ", ft_RotorRPM = " << output.ft_RotorRPM << ", ft_RotorDegreeRad = "
    << output.ft_RotorDegreeRad << ", ft_OutputTorque = " << output.ft_OutputTorque
    << std::endl;
}

idlMotorOutputMessageType ToIDLMotorOutputMessage(const MsgMotorOutput& output)
{
  return idlMotorOutputMessageType(output.ft_CurrentU, output.ft_CurrentV, output.ft_CurrentW,
    output.ft_RotorRPM, output.ft_RotorDegreeRad, output.ft_OutputTorque);
}

} // namespace

int main(int argc, char * argv[])
{
  /* DomainParticipant */
  ddsBridge.CreateDomainParticipant();

  /* Publisher */
  ddsBridge.AddPublisherPartition("MotorResponsePartition");
  ddsBridge.CreatePublisher();

  /* Subscriber */
  ddsBridge.AddSubscriberPartition("ControlCommandPartition");
  ddsBridge.AddSubscriberPartition("NodejsRequestPartition");
  ddsBridge.CreateSubscriber();

  /* DataWriter */
  auto writerQos = ddsBridge.CreateDataWriterQos();
  ddsBridge.AddQos(writerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));
  ddsBridge.AddQos(writerQos,
    dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances());

  auto motorDataWriter =
    ddsBridge.CreateDataWriter<idlMotorResponseMessageType>("motor_response_topic", writerQos);
  auto nodejsDataWriter =
    ddsBridge.CreateDataWriter<idlMotorOutputMessageType>("motor_output_topic", writerQos);

  /* DataReader */
  auto readerQos = ddsBridge.CreateDataReaderQos();
  ddsBridge.AddQos(readerQos,
    dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0)));

  auto controlDataReader =
    ddsBridge.CreateDataReader<idlControlCommandMessageType>("control_command_topic", readerQos);

  /* WaitSet */
  auto controlStatusCondition =
    ddsBridge.CreateStatusCondition<idlControlCommandMessageType>(controlDataReader);
  ddsBridge.EnableStatus(controlStatusCondition, dds::core::status::StatusMask::data_available());

  ddsBridge.CreateWaitSet();
  ddsBridge.AddStatusCondition(controlStatusCondition);
  ddsBridge.AddGuardCondition(ddsBridge.mTerminationGuard);

  std::string outputFilename;
  std::ofstream outputFile;

  if(argc == 0)
  {
    std::cerr << "Usage: motor [latency output]" << std::endl;
    return -1;
  }

  if(argc >= 2)
  {
    outputFilename = argv[1];
    std::cout << "Writing to " << outputFilename << " ..." << std::endl;
    outputFile.open(outputFilename);
    outputFile << "take,step,write" << std::endl;
  }

  generated_model_initialize();

  struct sigaction signalHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  dds::core::cond::WaitSet::ConditionSeq conditionSeq;

  std::cout << "Listening to Controller ..." << std::endl;

  auto beginTime = std::chrono::steady_clock::now();

  for(; !ddsBridge.mTerminationGuard.trigger_value();)
  {
    ddsBridge.mWaitSet.wait(conditionSeq);

    auto beginTakeTime = std::chrono::steady_clock::now();

    dds::sub::LoanedSamples<idlControlCommandMessageType> samples = controlDataReader.take();

    auto endTakeTime = std::chrono::steady_clock::now();

    for(auto sample = samples.begin(); sample < samples.end(); ++sample)
    {
      if(sample->info().valid())
      {
        idlMotorResponseMessageType motorMessage;

        if(sample->data().command() == "RequestMsgMotorOutput")
        {
          std::cout << "Received RequestMsgMotorOutput" << std::endl;

          auto msgMotorOutput = input_interface::GetMsgMotorOutput();

          std::cout << "Sending message with content: ";
          OutputMsgMotorOutput(msgMotorOutput);

          auto motorOutputMessage = idlMotorOutputMessageType(
            ToIDLMotorOutputMessage(msgMotorOutput));

          dds::core::InstanceHandle instanceHandle =
            nodejsDataWriter.register_instance(motorOutputMessage);

          nodejsDataWriter << motorOutputMessage;

          nodejsDataWriter.unregister_instance(instanceHandle);
        }
        else
        {
          auto beginMotorStepTime = std::chrono::steady_clock::now();

          motorStep();

          auto endMotorStepTime = std::chrono::steady_clock::now();

          ddsBridge.mMotorStepTimes.AddTime(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
              endMotorStepTime - beginMotorStepTime));

          motorMessage = idlMotorResponseMessageType("motor_step");

          dds::core::InstanceHandle instanceHandle =
            motorDataWriter.register_instance(motorMessage);

          auto beginWriteTime = std::chrono::steady_clock::now();

          motorDataWriter << motorMessage;

          auto endWriteTime = std::chrono::steady_clock::now();

          motorDataWriter.unregister_instance(instanceHandle);

          ddsBridge.mMotorWriteTimes.AddTime(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
              endWriteTime - beginWriteTime));
        }

      }
    }

    ddsBridge.mMotorTakeTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTakeTime - beginTakeTime));

    if(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - beginTime).count() >= 1)
    {
      ddsBridge.mMotorStepTimes.PrintHeader("motor");
      ddsBridge.mMotorTakeTimes.Print("take");
      ddsBridge.mMotorStepTimes.Print("step");
      ddsBridge.mMotorWriteTimes.Print("write");

      beginTime = std::chrono::steady_clock::now();
    }

    outputFile
      << ddsBridge.mMotorTakeTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << ddsBridge.mMotorStepTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << ddsBridge.mMotorWriteTimes.Back().count() / kMicrosecondsInOneNanosecond
      << std::endl;

    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  generated_model_terminate();

  if(!outputFilename.empty())
  {
    outputFile.close();
  }

  std::cout << "Motor Exiting ..." << std::endl;

  return 0;
}
