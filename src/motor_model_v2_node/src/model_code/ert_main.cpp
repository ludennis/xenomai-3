#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "generated_model.h"

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
      GetAverage() << std::endl;
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

  /* INSTANTIATION */
  generated_model_initialize();
  for (;;)
  {
    motorStep(motorStepElapsedTimes,
              publishElapsedTimes,
              totalElapsedTimes);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  generated_model_terminate();
  return 0;
}