#include <chrono>
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

  void Print(const std::string& title)
  {
    if(mTimes.size() == 0)
    {
      return;
    }
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

} // namespace utils