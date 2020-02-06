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
    : mSum(0)
    ,mMin(LLONG_MAX)
    ,mMax(LLONG_MIN)
  {};

  void AddTime(const std::chrono::nanoseconds &time)
  {
    mTimes.push_back(time);
    mSum += time.count();
    if(mMin > time.count())
      mMin = time.count();
    if(mMax < time.count())
      mMax = time.count();
  }

  long long int GetAverage()
  {

    return (mTimes.size() > 0) ? mSum / mTimes.size() : 0;
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

  void Print(const std::string& name)
  {
    if(mTimes.size() == 0)
    {
      return;
    }

    std::cout << std::left << std::setfill(' ') << std::setw(20) << name;
    std::cout << std::right << std::setfill(' ') << std::setw(20) << Back().count();
    std::cout << std::right << std::setfill(' ') << std::setw(20) << GetMax();
    std::cout << std::right << std::setfill(' ') << std::setw(20) << GetMin();
    std::cout << std::right << std::setfill(' ') << std::setw(20) << GetAverage();
    std::cout << std::endl << std::endl;
  }

  void PrintHeader(const std::string& entity)
  {
    std::cout << std::left << std::setfill('-') << std::setw(100) << '-' << std::endl;
    std::cout << std::left << std::setfill(' ') << std::setw(20)  << entity;
    std::cout << std::left << std::setfill(' ') << std::setw(20) << "Instance(ns)";
    std::cout << std::left << std::setfill(' ') << std::setw(20) << "Max(ns)";
    std::cout << std::left << std::setfill(' ') << std::setw(20) << "Min(ns)";
    std::cout << std::left << std::setfill(' ') << std::setw(20) << "Avg(ns)" << std::endl;
    std::cout << std::left << std::setfill('-') << std::setw(100) << '-' << std::endl;
  }

private:
  long long int mSum;
  long long int mMax;
  long long int mMin;
  std::vector<std::chrono::nanoseconds> mTimes;
};

} // namespace utils