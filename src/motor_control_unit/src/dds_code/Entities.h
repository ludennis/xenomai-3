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

  /* General */
  template <typename EntityType, typename PolicyType>
  void AddQos(EntityType &entity, PolicyType policy)
  {
    entity << policy;
  }

  /* DomainParticipant */
  void CreateDomainParticipant();
  void CreateDomainParticipant(unsigned int id);

  /* Publisher */
  void CreatePublisher();
  void AddPublisherPartition(const std::string &partition);

  /* Subscriber */
  void CreateSubscriber();
  void AddSubscriberPartition(const std::string &partition);

  /* DataWriter */
  dds::pub::qos::DataWriterQos CreateDataWriterQos();

  template <typename MessageType>
  dds::pub::DataWriter<MessageType> CreateDataWriter(const std::string &topicName,
    const dds::pub::qos::DataWriterQos &qos)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataWriter = dds::pub::DataWriter<MessageType>(this->mPublisher, topic, qos);
    return dataWriter;
  }

  /* DataReader */
  dds::sub::qos::DataReaderQos CreateDataReaderQos();

  template <typename MessageType>
  dds::sub::DataReader<MessageType> CreateDataReader(const std::string &topicName,
    const dds::sub::qos::DataReaderQos &qos)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataReader = dds::sub::DataReader<MessageType>(this->mSubscriber, topic, qos);
    return dataReader;
  }

  template <typename MessageType, typename PolicyType>
  void AddDataReaderQos(dds::sub::DataReader<MessageType> &reader, PolicyType policy)
  {
    auto readerQos = dds::sub::qos::DataReaderQos();
    readerQos << policy;
    reader << readerQos;
  }

  /* WaitSet */
  template <typename MessageType>
  dds::core::cond::StatusCondition CreateStatusCondition(
    dds::sub::DataReader<MessageType> reader)
  {
    dds::core::cond::StatusCondition statusCondition(reader);
    return statusCondition;
  }

  template <typename StatusMaskType>
  void EnableStatus(dds::core::cond::StatusCondition &statusCondition,
    StatusMaskType mask)
  {
    dds::core::status::StatusMask statusMask;
    statusMask << mask;
    statusCondition.enabled_statuses(statusMask);
  }

  void CreateWaitSet();

  void AddStatusCondition(dds::core::cond::StatusCondition &statusCondition);

  void AddGuardCondition(dds::core::cond::GuardCondition &guardCondition);

public:
  dds::domain::DomainParticipant mParticipant;

  dds::pub::Publisher mPublisher;
  dds::sub::Subscriber mSubscriber;

  dds::core::cond::WaitSet mWaitSet;

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
