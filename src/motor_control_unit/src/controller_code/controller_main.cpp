#include <fstream>
#include <thread>
#include <signal.h>

#include <dds/dds.hpp>

#include <dds_code/Entities.h>
#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

static dds_entities::Entities entities("controller", "motor");

void TerminationHandler(int s)
{
  std::cout << "Caught Termination Signal" << std::endl;
  entities.mTerminationGuard.trigger_value(true);
}

int main(int argc, char *argv[])
{
  std::string outputFilename;
  std::ofstream outputFile;

  if (argc == 0)
  {
    std::cerr << "Usage: controller [latency output]" << std::endl;
    return -1;
  }

  if (argc >= 2)
  {
    outputFilename = argv[1];
    std::cout << "Writing to " << outputFilename << " ..." << std::endl;
    outputFile.open(outputFilename);
    outputFile << "write,roundtrip,take" << std::endl;
  }

  // signal handler for ctrl + c
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  // conditions for waitisets
  dds::core::cond::WaitSet::ConditionSeq conditionSeq;
  dds::core::Duration waitSetConnectionTimeout(5, 0);
  dds::core::Duration waitSetTransmissionTimeout(1, 0);

  std::cout << "Connecting to motor ...";
  for(auto count{1ll};;++count)
  {
    entities.mControlMessageWriter <<
      MotorControllerUnitModule::ControlMessage(count, "connect");
    try
    {
      entities.mMotorMessageWaitSet.wait(conditionSeq, waitSetConnectionTimeout);
      entities.mMotorMessageReader.take();
      std::cout << "Connected!" << std::endl;
      break;
    }
    catch(dds::core::TimeoutError &e)
    {
      (void)e;
    }
  }

  auto beginTime = std::chrono::steady_clock::now();
  for(auto count{1ll}; !entities.mTerminationGuard.trigger_value();++count)
  {
    auto controlMessage = MotorControllerUnitModule::ControlMessage(count, "motor_step");

    dds::core::InstanceHandle instanceHandle =
      entities.mControlMessageWriter.register_instance(controlMessage);

    auto beginWriteTime = std::chrono::steady_clock::now();

    entities.mControlMessageWriter << controlMessage;

    auto endWriteTime = std::chrono::steady_clock::now();

    entities.mControlMessageWriter.unregister_instance(instanceHandle);

    entities.mMotorMessageWaitSet.wait(conditionSeq, waitSetTransmissionTimeout);

    auto beginTakeTime = std::chrono::steady_clock::now();

    auto samples = entities.mMotorMessageReader.take();

    auto endTakeTime = std::chrono::steady_clock::now();

    entities.mControllerWriteTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endWriteTime - beginWriteTime));

    entities.mControllerTakeTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTakeTime - beginTakeTime));

    entities.mRoundTripTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTakeTime - beginWriteTime));

    if(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - beginTime).count() >= 1)
    {
      entities.mRoundTripTimes.PrintHeader("controller");
      entities.mControllerWriteTimes.Print("write");
      entities.mRoundTripTimes.Print("roundtrip");
      entities.mControllerTakeTimes.Print("take");

      beginTime = std::chrono::steady_clock::now();
    }

    outputFile
      << entities.mControllerWriteTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << entities.mRoundTripTimes.Back().count() / kMicrosecondsInOneNanosecond << ","
      << entities.mControllerWriteTimes.Back().count() / kMicrosecondsInOneNanosecond
      << std::endl;

    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  if(!outputFilename.empty())
  {
    outputFile.close();
  }


  std::cout << "Controller Exiting ..." << std::endl;

  return 0;
}
