#include <thread>

#include <dds/dds.hpp>

#include <dds_code/Entities.h>
#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

int main()
{
  dds_entities::Entities entities("controller", "motor");

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
  for(auto count{1ll};;++count)
  {
    auto beginWriteTime = std::chrono::steady_clock::now();

    entities.mControlMessageWriter <<
      MotorControllerUnitModule::ControlMessage(count, "motor_step");

    auto endWriteTime = std::chrono::steady_clock::now();

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
  }

  return 0;
}
