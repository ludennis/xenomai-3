#ifndef __ELAPSEDTIMES_HPP__
#define __ELAPSEDTIMES_HPP__

#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

namespace utils
{

class ElapsedTimes

{
public:
  ElapsedTimes()
    : mSize(0)
    , mSum(0)
    , mMin(LLONG_MAX)
    , mMax(LLONG_MIN)
    , mBack(std::chrono::nanoseconds())
  {};

  void AddTime(const std::chrono::nanoseconds &time)
  {
    mSize++;
    mSum += time.count();
    if(mMin > time.count())
      mMin = time.count();
    if(mMax < time.count())
      mMax = time.count();
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

  long long int GetMax()
  {
    return mMax;
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
      GetMax() / kMicrosecondsInOneNanosecond;
    std::cout << std::right << std::setfill(' ') << std::setw(15) <<
      GetSize();
    std::cout << std::endl << std::endl;
  }

  void PrintHeader(const std::string& entity)
  {
    std::cout << std::right << std::setfill('-') << std::setw(90) << '-' << std::endl;
    std::cout << std::right << std::setfill(' ') << std::setw(15)  << entity;
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Instance(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Min(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Avg(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Max(us)";
    std::cout << std::right << std::setfill(' ') << std::setw(15) << "Count";
    std::cout << std::endl;
    std::cout << std::right << std::setfill('-') << std::setw(90) << '-' << std::endl;
  }

private:
  long int mSize;
  long long int mSum;
  long long int mMax;
  long long int mMin;
  std::chrono::nanoseconds mBack;
};

} // namespace utils

#endif // __ELAPSEDTIMES_HPP__