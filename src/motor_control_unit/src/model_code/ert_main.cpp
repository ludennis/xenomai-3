#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>

#include <dds_code/Entities.h>
#include "generated_model.h"
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

static dds_entities::Entities entities;

void motorStep()
{
  generated_model_step();
}

int main(int argc, char * argv[])
{
  entities.CreateDomainParticipant();

  entities.AddPublisherPartition("motorPartition");
  entities.AddSubscriberPartition("controlPartition");
  entities.AddSubscriberPartition("nodejsPartition");

  entities.CreatePublisher();
  entities.CreateSubscriber();

  auto controlDataWriter =
    entities.CreateDataWriter<MotorControllerUnitModule::ControlMessage>("control_topic");

  auto motorDataReader =
    entities.CreateDataReader<MotorControllerUnitModule::MotorMessage>("motor_topic");

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

  for(; !entities.mTerminationGuard.trigger_value();)
  {
    entities.mControlMessageWaitSet.wait(conditionSeq);

    auto beginTakeTime = std::chrono::steady_clock::now();

    dds::sub::LoanedSamples<MotorControllerUnitModule::ControlMessage> samples =
      entities.mControlMessageReader.take();

    auto endTakeTime = std::chrono::steady_clock::now();

    for(auto sample = samples.begin(); sample < samples.end(); ++sample)
    {
      if(sample->info().valid())
      {
        auto beginMotorStepTime = std::chrono::steady_clock::now();

        motorStep();

        auto endMotorStepTime = std::chrono::steady_clock::now();

        entities.mMotorStepTimes.AddTime(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            endMotorStepTime - beginMotorStepTime));

        auto motorMessage =
          MotorControllerUnitModule::MotorMessage("motor_step");

        dds::core::InstanceHandle instanceHandle =
          entities.mMotorMessageWriter.register_instance(motorMessage);

        auto beginWriteTime = std::chrono::steady_clock::now();

        entities.mMotorMessageWriter << motorMessage;

        auto endWriteTime = std::chrono::steady_clock::now();

        entities.mMotorMessageWriter.unregister_instance(instanceHandle);

        entities.mMotorWriteTimes.AddTime(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            endWriteTime - beginWriteTime));
      }
    }

    entities.mMotorTakeTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTakeTime - beginTakeTime));

    if(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - beginTime).count() >= 1)
    {
      entities.mMotorStepTimes.PrintHeader("motor");
      entities.mMotorTakeTimes.Print("take");
      entities.mMotorStepTimes.Print("step");
      entities.mMotorWriteTimes.Print("write");

      beginTime = std::chrono::steady_clock::now();
    }

    outputFile
      << entities.mMotorTakeTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << entities.mMotorStepTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << entities.mMotorWriteTimes.Back().count() / kMicrosecondsInOneNanosecond
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
