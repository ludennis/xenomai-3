#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

//static dds::core::cond::GuardCondition terminated;

namespace dds_entities
{

class Entities
{
public:
  Entities(const std::string &publisherPartition, const std::string &subscriberPartition);

public:
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage> mControlMessageWriter;
  dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage> mMotorMessageWriter;

  dds::sub::DataReader<MotorControllerUnitModule::ControlMessage> mControlMessageReader;
  dds::sub::DataReader<MotorControllerUnitModule::MotorMessage> mMotorMessageReader;

  dds::core::cond::WaitSet mMotorMessageWaitSet;
  dds::core::cond::WaitSet mControlMessageWaitSet;

  utils::ElapsedTimes mRoundTripTimes;
  utils::ElapsedTimes mControllerWriteTimes;
  utils::ElapsedTimes mControllerTakeTimes;
  utils::ElapsedTimes mMotorWriteTimes;
  utils::ElapsedTimes mMotorTakeTimes;
  utils::ElapsedTimes mMotorStepTimes;

  dds::core::cond::GuardCondition mTerminationGuard;
};

} // namespace dds_entities
