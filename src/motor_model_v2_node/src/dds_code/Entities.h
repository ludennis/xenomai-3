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

private:
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage> mControlWriter;
  dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage> mMotorWriter;
  dds::sub::DataReader<MotorControllerUnitModule::ControlMessage> mControlReader;
  dds::sub::DataReader<MotorControllerUnitModule::MotorMessage> mMotorReader;
  dds::core::cond::WaitSet mWaitSet;
  utils::ElapsedTimes roundTripElapsedTimes;
  utils::ElapsedTimes motorStepElapsedTimes;
  utils::ElapsedTimes publishElapsedTimes;
  utils::ElapsedTimes totalElapsedTimes;
};

} // namespace dds_entities