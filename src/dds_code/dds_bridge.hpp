#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

namespace dds_bridge
{

class DDSBridge
{
public:
  DDSBridge()
  : mParticipant(dds::core::null)
  , mPublisher(dds::core::null)
  , mSubscriber(dds::core::null)
  , mWaitSet(dds::core::null)
  {}

  /* General */
  template <typename EntityType, typename PolicyType>
  void AddQos(EntityType &entity, PolicyType policy)
  {
    entity << policy;
  }

  /* DomainParticipant */
  void CreateDomainParticipant()
  {
    this->mParticipant = dds::domain::DomainParticipant(org::opensplice::domain::default_id());
  }

  void CreateDomainParticipant(const unsigned int id)
  {
    this->mParticipant = dds::domain::DomainParticipant(id);
  }

  /* Publisher */
  void CreatePublisher()
  {
    if(this->mPublisherPartitions.empty())
    {
      this->mPublisher = dds::pub::Publisher(this->mParticipant);
    }
    else
    {
      dds::pub::qos::PublisherQos publisherQos =
        this->mParticipant.default_publisher_qos() <<
          dds::core::policy::Partition(this->mPublisherPartitions);
      this->mPublisher = dds::pub::Publisher(this->mParticipant, publisherQos);
    }
  }

  void AddPublisherPartition(const std::string &partition)
  {
    this->mPublisherPartitions.push_back(partition);
  }

  /* Subscriber */
  void CreateSubscriber()
  {
    if(this->mSubscriberPartitions.empty())
    {
      this->mSubscriber = dds::sub::Subscriber(this->mParticipant);
    }
    else
    {
      dds::sub::qos::SubscriberQos subscriberQos =
        this->mParticipant.default_subscriber_qos() <<
          dds::core::policy::Partition(this->mSubscriberPartitions);
      this->mSubscriber = dds::sub::Subscriber(this->mParticipant, subscriberQos);
    }
  }

  void AddSubscriberPartition(const std::string &partition)
  {
    this->mSubscriberPartitions.push_back(partition);
  }

  /* DataWriter */
  dds::pub::qos::DataWriterQos CreateDataWriterQos()
  {
    dds::pub::qos::DataWriterQos writerQos;
    return writerQos;
  }

  template <typename MessageType>
  dds::pub::DataWriter<MessageType> CreateDataWriter(const std::string &topicName,
    const dds::pub::qos::DataWriterQos &qos)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataWriter = dds::pub::DataWriter<MessageType>(this->mPublisher, topic, qos);
    return dataWriter;
  }

  /* DataReader */
  dds::sub::qos::DataReaderQos CreateDataReaderQos()
  {
    dds::sub::qos::DataReaderQos readerQos;
    return readerQos;
  }

  template <typename MessageType>
  dds::sub::DataReader<MessageType> CreateDataReader(const std::string &topicName,
    const dds::sub::qos::DataReaderQos &qos)
  {
    dds::topic::Topic<MessageType> topic(this->mParticipant, topicName);
    auto dataReader = dds::sub::DataReader<MessageType>(this->mSubscriber, topic, qos);
    return dataReader;
  }

  /* WaitSet */
  void CreateWaitSet()
  {
    this->mWaitSet = dds::core::cond::WaitSet();
  }

  void AddStatusCondition(dds::core::cond::StatusCondition &statusCondition)
  {
    this->mWaitSet += statusCondition;
  }

  void AddGuardCondition(dds::core::cond::GuardCondition &guardCondition)
  {
    this->mWaitSet += guardCondition;
  }

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

}; // class DDSBridge

} // namespace dds_bridge
