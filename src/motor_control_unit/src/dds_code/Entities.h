#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

//static dds::core::cond::GuardCondition terminated;

namespace dds_entities
{

class Entities
{
public:
  Entities();

  void AddPublisherPartition(const std::string &partition);
  void AddSubscriberPartition(const std::string &partition);

  void CreatePublisher();
  void CreateSubscriber();

  template <typename MessageType>
  dds::pub::DataWriter<MessageType> CreateDataWriter(const std::string &topicName)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataWriter = dds::pub::DataWriter<MessageType>(this->mPublisher, topic);
    return dataWriter;
  }

  template <typename MessageType>
  dds::sub::DataReader<MessageType> CreateDataReader(const std::string &topicName)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataReader = dds::sub::DataReader<MessageType>(this->mSubscriber, topic);
    return dataReader;
  }

public:
  dds::domain::DomainParticipant mParticipant;

  dds::pub::Publisher mPublisher;
  dds::sub::Subscriber mSubscriber;

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

  std::vector<std::string> mPublisherPartitions;
  std::vector<std::string> mSubscriberPartitions;
};

} // namespace dds_entities
