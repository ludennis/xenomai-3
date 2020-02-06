#include <algorithm>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>

#include "generated_model.h"
#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

static dds::core::cond::GuardCondition terminated;

void motorStep()
{
  generated_model_step();
}

int main(int argc, char * argv[])
{
  generated_model_initialize();

  utils::ElapsedTimes motorStepElapsedTimes;
  utils::ElapsedTimes publishElapsedTimes;
  utils::ElapsedTimes totalElapsedTimes;

  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());

  dds::topic::Topic<MotorControllerUnitModule::ControlMessage>
    controlTopic(participant, "control_topic");
  dds::topic::Topic<MotorControllerUnitModule::MotorMessage>
    motorTopic(participant,"motor_topic");

  dds::sub::Subscriber subscriber(participant);
  dds::sub::DataReader<MotorControllerUnitModule::ControlMessage>
    reader(subscriber, controlTopic);

  dds::pub::Publisher publisher(participant);
  dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage>
    motorMessageWriter(publisher,motorTopic);

  dds::core::cond::WaitSet waitSet;
  waitSet = dds::core::cond::WaitSet();
  dds::core::cond::StatusCondition dataAvailable(reader);
  dds::core::status::StatusMask statusMask;
  statusMask << dds::core::status::StatusMask::data_available();
  dataAvailable.enabled_statuses(statusMask);
  waitSet += dataAvailable;
  waitSet += terminated;
  dds::core::cond::WaitSet::ConditionSeq conditionSeq;
  dds::core::Duration waitSetTransmissionTimeout(1, 0);
  dds::core::Duration waitSetConnectionTimeout(25, 0);

  for(auto count{1ll};;++count)
  {
    std::cout << "Waiting for control message ..." << std::endl;
    try
    {
      waitSet.wait(conditionSeq,waitSetTransmissionTimeout);
    }
    catch(dds::core::TimeoutError &e)
    {
      (void)e;
      continue;
    }

    auto beginTotalTime = std::chrono::high_resolution_clock::now();
    dds::sub::LoanedSamples<MotorControllerUnitModule::ControlMessage> samples =
      reader.take();

    for(auto sample = samples.begin(); sample < samples.end(); ++sample)
    {
      if(sample->info().valid())
      {
        std::cout << "Received control message: " << sample->data().content() <<
          std::endl;
        if(sample->data().content() == "motor_step")
        {
          auto beginMotorStepTime = std::chrono::high_resolution_clock::now();

          motorStep();

          auto endMotorStepTime = std::chrono::high_resolution_clock::now();
          motorStepElapsedTimes.AddTime(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
              endMotorStepTime - beginMotorStepTime));
        }
      }
    }

    auto beginPublishTime = std::chrono::high_resolution_clock::now();
    motorMessageWriter << MotorControllerUnitModule::MotorMessage(count, "Stepped!");

    std::cout << "Sending step completion message to controller ..." << std::endl;

    auto endPublishTime = std::chrono::high_resolution_clock::now();
    auto endTotalTime = std::chrono::high_resolution_clock::now();

    publishElapsedTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endPublishTime - beginPublishTime));

    totalElapsedTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTotalTime - beginTotalTime));

    motorStepElapsedTimes.PrintHeader("motor");
    motorStepElapsedTimes.Print("motorStep");
    publishElapsedTimes.Print("publish");
    totalElapsedTimes.Print("total");
  }

  generated_model_terminate();
  return 0;
}