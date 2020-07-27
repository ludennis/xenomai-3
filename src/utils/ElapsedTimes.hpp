#ifndef __ELAPSEDTIMES_HPP__
#define __ELAPSEDTIMES_HPP__

#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

constexpr auto kMicrosecondsInOneNanosecond = 1000.;

namespace utils
{

class ElapsedTimes
{
public:
  ElapsedTimes()
    : mSize(0)
    , mSum(0)
    , mMin(LLONG_MAX)
    , mLocalMax(LLONG_MIN)
    , mGlobalMax(LLONG_MIN)
    , mBack(std::chrono::nanoseconds())
  {};

  void AddTime(const std::chrono::nanoseconds &time)
  {
    mSize++;
    mSum += time.count();
    if(mMin > time.count())
      mMin = time.count();
    if(mLocalMax < time.count())
      mLocalMax = time.count();
    if(mGlobalMax < time.count())
      mGlobalMax = time.count();
    mBack = time;
  }

  long long int GetAverage()
  {
    return (mSize > 0) ? mSum / mSize : 0;
  }

  long long int GetSum()
  {
    return mSum;
  }

  long long int GetMin()
  {
    return mMin;
  }

  long long int GetLocalMax()
  {
    return mLocalMax;
  }

  long long int GetGlobalMax()
  {
    return mGlobalMax;
  }

  std::chrono::nanoseconds Back()
  {
    return mBack;
  }

  long int GetSize()
  {
    return mSize;
  }

  void Print(const std::string& name)
  {
    if(mSize == 0)
    {
      return;
    }

    std::cout << std::right << std::setfill(' ') << std::setw(15) << name;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      Back().count() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetMin() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetAverage() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetLocalMax() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetGlobalMax() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetSize();
    std::cout << std::endl << std::endl;

    mLocalMax = LLONG_MIN;
  }

  void PrintHeader(const std::string& entity)
  {
    std::cout << std::right << std::setfill('-') << std::setw(105) << '-' << std::endl;
    std::cout << std::right << std::setfill(' ') << std::setw(15)  << entity;
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Instance(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Min(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Avg(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "LocalMax(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "GlobalMax(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Count";
    std::cout << std::endl;
    std::cout << std::right << std::setfill('-') << std::setw(105) << '-' << std::endl;
  }

private:
  long int mSize;
  long long int mSum;
  long long int mMin;
  long long int mLocalMax;
  long long int mGlobalMax;
  std::chrono::nanoseconds mBack;
};

} // namespace utils

#endif // __ELAPSEDTIMES_HPP__
