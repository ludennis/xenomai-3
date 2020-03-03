#ifndef __FILE_WRITER_HPP__
#define __FILE_WRITER_HPP__

#include <memory>
#include <string>

#include <utils/SynchronizedFile.hpp>

class FileWriter
{
public:
  FileWriter(std::shared_ptr<SynchronizedFile> synchronizedFile)
  : mSynchronizedFile(synchronizedFile)
  {}

  FileWriter(std::shared_ptr<SynchronizedFile> synchronizedFile, const std::string& dataToWrite)
  : mSynchronizedFile(synchronizedFile)
  , mDataToWrite(dataToWrite)
  {}

  void flush()
  {
    mSynchronizedFile->write(mDataToWrite);
    mDataToWrite.clear();
  }

  void write(const std::string dataToWrite)
  {
    mSynchronizedFile->write(dataToWrite);
  }

  void setDataToWrite(const std::string& dataToWrite)
  {
    mDataToWrite = dataToWrite;
  }

private:
  std::shared_ptr<SynchronizedFile> mSynchronizedFile;
  std::string mDataToWrite;
};

#endif // __FILE_WRITER_HPP__
