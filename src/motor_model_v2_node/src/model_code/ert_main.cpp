#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "generated_model.h"

#include <dds/dds.hpp>
#include <MotorControllerUnitModule_DCPS.hpp>

static dds::core::cond::GuardCondition terminated;

class ElapsedTimes
{
public:
  ElapsedTimes()
    : mSum(0)
    , mMin(LLONG_MAX)
    , mMax(LLONG_MIN)
  {};

  void AddTime(const std::chrono::nanoseconds &time)
  {
    mTimes.push_back(time);
    mSum += time.count();
    if (mMin > time.count())
      mMin = time.count();
    if (mMax < time.count())
      mMax = time.count();
  }

  long long int GetAverage()
  {
    return mSum / mTimes.size();
  }

  long long int GetSum()
  {
    return mSum;
  }

  long long int GetMin()
  {
    return mMin;
  }

  long long int GetMax()
  {
    return mMax;
  }

  std::chrono::nanoseconds Back()
  {
    return mTimes.back();
  }

  void Print(const std::string& title)
  {
    std::cout << title << "\t\t\t" << Back().count() <<
      "\t\t" << GetMax() << "\t\t" << GetMin() << "\t" <<
      GetAverage() << std::endl << std::endl;
  }

private:
  long long int mSum;
  long long int mMax;
  long long int mMin;
  std::vector<std::chrono::nanoseconds> mTimes;
};

void motorStep(ElapsedTimes& motorStepElapsedTimes,
  ElapsedTimes& publishElapsedTimes, ElapsedTimes& totalElapsedTimes)
{
  auto beginTime = std::chrono::steady_clock::now();

  // motor step
  generated_model_step();

  auto endMotorTime = std::chrono::steady_clock::now();

  // publish
  auto endPublishTime = std::chrono::steady_clock::now();

  motorStepElapsedTimes.AddTime(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      endMotorTime - beginTime));
  publishElapsedTimes.AddTime(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      endPublishTime - endMotorTime));
  totalElapsedTimes.AddTime(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      endPublishTime - beginTime));

  printf("------------------------------------------------------------------------\n");
  printf("Measured duration\tInstant(ns)\tMax(ns)\t\tMin(ns)\tAvg(ns)\n");
  printf("------------------------------------------------------------------------\n");
  motorStepElapsedTimes.Print("motor");
  publishElapsedTimes.Print("publish");
  totalElapsedTimes.Print("total");

}

int main(int argc, char * argv[])
{
  ElapsedTimes motorStepElapsedTimes;
  ElapsedTimes publishElapsedTimes;
  ElapsedTimes totalElapsedTimes;

  // subscriber for control message
  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage>
    controlTopic(participant, "control_topic");
  dds::sub::Subscriber subscriber(participant);
  dds::sub::DataReader<MotorControllerUnitModule::ControlMessage>
    reader(subscriber, controlTopic);

  // waitset for subscriber
  dds::core::cond::WaitSet waitSet;
  waitSet = dds::core::cond::WaitSet();

  // status conditions for waitset
  dds::core::cond::StatusCondition dataAvailable(reader);
  dds::core::status::StatusMask statusMask;
  statusMask << dds::core::status::StatusMask::data_available();
  dataAvailable.enabled_statuses(statusMask);
  waitSet += dataAvailable;
  waitSet += terminated;

  dds::core::cond::WaitSet::ConditionSeq conditions;
  dds::core::Duration waitTimeout(1, 0);

  /* INSTANTIATION */
  generated_model_initialize();
  for (;;)
  {
    std::cout << "Waiting for control message ..." << std::endl;
    waitSet.wait(conditions, waitTimeout);
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
          motorStep(motorStepElapsedTimes, publishElapsedTimes, totalElapsedTimes);
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  generated_model_terminate();
  return 0;
}