#include <algorithm>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>

#include <dds_code/Entities.h>
#include "generated_model.h"
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

static dds_entities::Entities entities("motor", "controller");

void motorStep()
{
  generated_model_step();
}

void TerminationHandler(int s)
{
  std::cout << "Caught Termination Signal" << std::endl;
  entities.mTerminationGuard.trigger_value(true);
}

int main(int argc, char * argv[])
{
  generated_model_initialize();

  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
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
          MotorControllerUnitModule::MotorMessage(sample->data().id(), "motor_step");

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
  }

  generated_model_terminate();

  std::cout << "Motor Exiting ..." << std::endl;

  return 0;
}
