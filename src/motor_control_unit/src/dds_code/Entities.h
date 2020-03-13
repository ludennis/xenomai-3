#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

//static dds::core::cond::GuardCondition terminated;

namespace dds_entities
{

class Entities
{
public:
  Entities(const std::vector<std::string> &publisherPartitions,
    const std::vector<std::string> &subscriberPartitions);

public:
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage> mControlMessageWriter;
  dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage> mMotorMessageWriter;

  dds::sub::DataReader<MotorControllerUnitModule::ControlMessage> mControlMessageReader;
  dds::sub::DataReader<MotorControllerUnitModule::MotorMessage> mMotorMessageReader;

  dds::core::cond::WaitSet mControlMessageWaitSet;
  dds::core::cond::WaitSet mMotorMessageWaitSet;

  utils::ElapsedTimes mRoundTripTimes;
  utils::ElapsedTimes mControllerWriteTimes;
  utils::ElapsedTimes mControllerTakeTimes;
  utils::ElapsedTimes mMotorWriteTimes;
  utils::ElapsedTimes mMotorTakeTimes;
  utils::ElapsedTimes mMotorStepTimes;

  dds::core::cond::GuardCondition mTerminationGuard;
};

} // namespace dds_entities
