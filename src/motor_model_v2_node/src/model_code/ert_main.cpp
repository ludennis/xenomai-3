#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "generated_model.h"

class ElapsedTimes
{
public:
  ElapsedTimes()
    : mSum(0)
    , mMin(-999)
    , mMax(999)
  {};

  void AddTime(const std::chrono::nanoseconds &time)
  {
    mTimes.push_back(time);
    mSum += time;
    if (mMin > time)
      mMin = time;
    if (mMax < time)
      mMax = time;
  }

  std::chrono::nanoseconds GetAverage()
  {
    return mSum / mTimes.size();
  }

  long int GetSum()
  {
    return mSum.count();
  }

  std::chrono::nanoseconds GetMin()
  {
    return mMin;
  }

  std::chrono::nanoseconds GetMax()
  {
    return mMax;
  }

  std::chrono::nanoseconds Back()
  {
    return mTimes.back();
  }

  void Print()
  {
    printf("Publish\t\t\t%d\t\t%d\t\t%d\t%d\n",
      Back().count(),
      GetMax().count(),
      GetMin().count(),
      GetAverage().count());
  }

private:
  std::chrono::nanoseconds mSum;
  std::chrono::nanoseconds mMax;
  std::chrono::nanoseconds mMin;
  std::vector<std::chrono::nanoseconds> mTimes;
};

ElapsedTimes motorStepElapsedTimes;
ElapsedTimes publishElapsedTimes;
ElapsedTimes totalElapsedTimes;

void motorStep()
{
  auto beginTime = std::chrono::steady_clock::now();

  // motor step
  std::cout << "Generated_model_step()!" << std::endl;
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

  printf("\e[2J\e[H");
  printf("------------------------------------------------------------------------\n");
  printf("Measured duration\tInstant(ns)\tMax(ns)\t\tMin(ns)\tAvg(ns)\n");
  printf("------------------------------------------------------------------------\n");
  motorStepElapsedTimes.Print();
  publishElapsedTimes.Print();
  totalElapsedTimes.Print();

}

int main(int argc, char * argv[])
{
  /* INSTANTIATION */
  std::cout << "Initializing Generated Model!" << std::endl;
  generated_model_initialize();
  for (;;)
  {
    std::cout << "Motor Stepping!" << std::endl;
    motorStep();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  std::cout << "Terminating model!" << std::endl;
  generated_model_terminate();
  return 0;
}
