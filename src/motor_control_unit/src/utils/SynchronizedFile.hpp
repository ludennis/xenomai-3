#ifndef __SYNCHRONIZED_FILE_HPP__
#define __SYNCHRONIZED_FILE_HPP__

#include <mutex>
#include <string>

class SynchronizedFile
{
public:
  SynchronizedFile()
  {}

  SynchronizedFile(const std::string& path)
  : mPath(path)
  {}

  void write(const std::string dataToWrite)
  {
    {
      std::lock_guard<std::mutex> lock(mWriterMutex);
      mFile.open(mPath, std::ofstream::out | std::ofstream::app);
      mFile << dataToWrite;
      mFile.close();
    }
  }

  void open(const std::string& path)
  {
    mPath = path;
    mFile.open(mPath);
  }

  void close()
  {
    mFile.close();
  }

private:
  std::ofstream mFile;
  std::string mPath;
  std::mutex mWriterMutex;
};

#endif // __SYNCHRONIZED_FILE_HPP__
