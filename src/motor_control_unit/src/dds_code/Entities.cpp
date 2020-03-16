#include <Entities.h>

namespace dds_entities
{

Entities::Entities()
  : mParticipant(dds::core::null)
  , mPublisher(dds::core::null)
  , mSubscriber(dds::core::null)
  , mControlMessageWriter(dds::core::null)
  , mMotorMessageWriter(dds::core::null)
  , mControlMessageReader(dds::core::null)
  , mMotorMessageReader(dds::core::null)
  , mControlMessageWaitSet(dds::core::null)
  , mMotorMessageWaitSet(dds::core::null)
{
  // Domain
  this->mParticipant = dds::domain::DomainParticipant(org::opensplice::domain::default_id());

  // Topic
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage>
    controlTopic(this->mParticipant, "control_topic");
  dds::topic::Topic<MotorControllerUnitModule::MotorMessage>
    motorTopic(this->mParticipant, "motor_topic");

  // Publisher
  this->mPublisher = dds::pub::Publisher(this->mParticipant);

  // DataWriter
  dds::pub::qos::DataWriterQos writerQos = controlTopic.qos();
  dds::pub::qos::DataWriterQos motorWriterQos = motorTopic.qos();
  writerQos << dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0))
    << dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
  mControlMessageWriter = dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage>(
    this->mPublisher, controlTopic, writerQos);
  mMotorMessageWriter = dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage>(
    this->mPublisher, motorTopic, writerQos);

  // Subscriber
  this->mSubscriber = dds::sub::Subscriber(this->mParticipant);

  // DataReader
  dds::sub::qos::DataReaderQos readerQos = controlTopic.qos();
  readerQos << dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0));
  mControlMessageReader = dds::sub::DataReader<MotorControllerUnitModule::ControlMessage>(
    this->mSubscriber, controlTopic, readerQos);
  mMotorMessageReader = dds::sub::DataReader<MotorControllerUnitModule::MotorMessage>(
    this->mSubscriber, motorTopic, readerQos);

  // StatusCondition
  dds::core::cond::StatusCondition controlDataAvailable(mControlMessageReader);
  dds::core::cond::StatusCondition motorDataAvailable(mMotorMessageReader);
  dds::core::status::StatusMask statusMask;
  statusMask << dds::core::status::StatusMask::data_available();
  controlDataAvailable.enabled_statuses(statusMask);
  motorDataAvailable.enabled_statuses(statusMask);

  // WaitSet
  mControlMessageWaitSet = dds::core::cond::WaitSet();
  mControlMessageWaitSet += controlDataAvailable;
  mControlMessageWaitSet += mTerminationGuard;
  mMotorMessageWaitSet = dds::core::cond::WaitSet();
  mMotorMessageWaitSet += motorDataAvailable;
  mMotorMessageWaitSet += mTerminationGuard;
}

void Entities::AddPublisherPartition(const std::string &partition)
{
  this->mPublisherPartitions.push_back(partition);
}

void Entities::AddSubscriberPartition(const std::string &partition)
{
  this->mSubscriberPartitions.push_back(partition);
}

void Entities::CreatePublisher()
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
    this->mPublisher << publisherQos;
  }
}

void Entities::CreateSubscriber()
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
    this->mSubscriber << subscriberQos;
  }
}

} // namespace dds_entities
