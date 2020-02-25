#include <Entities.h>

namespace dds_entities
{

Entities::Entities(const std::string &publisherPartition, 
  const std::string &subscriberPartition)
  : mControlMessageWriter(dds::core::null)
  , mMotorMessageWriter(dds::core::null)
  , mControlMessageReader(dds::core::null)
  , mMotorMessageReader(dds::core::null)
  , mControlMessageWaitSet(dds::core::null)
  , mMotorMessageWaitSet(dds::core::null)
{
  // Domain
  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());

  // Topic
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage> 
    controlTopic(participant, "control_topic");
  dds::topic::Topic<MotorControllerUnitModule::MotorMessage>
    motorTopic(participant, "motor_topic");
  
  // Publisher
  dds::pub::qos::PublisherQos publisherQos =
    participant.default_publisher_qos() <<
      dds::core::policy::Partition(publisherPartition);
  dds::pub::Publisher publisher(participant, publisherQos);

  // DataWriter
  dds::pub::qos::DataWriterQos writerQos = controlTopic.qos();
  dds::pub::qos::DataWriterQos motorWriterQos = motorTopic.qos();
  writerQos << dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0))
    << dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
  mControlMessageWriter = dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage>(
    publisher, controlTopic, writerQos);
  mMotorMessageWriter = dds::pub::DataWriter<MotorControllerUnitModule::MotorMessage>(
    publisher, motorTopic, writerQos);

  // Subscriber
  dds::sub::qos::SubscriberQos subscriberQos =
    participant.default_subscriber_qos() <<
      dds::core::policy::Partition(subscriberPartition);
  dds::sub::Subscriber subscriber(participant, subscriberQos);

  // DataReader
  dds::sub::qos::DataReaderQos readerQos = controlTopic.qos();
  readerQos << dds::core::policy::Reliability::Reliable(dds::core::Duration(10, 0));
  mControlMessageReader = dds::sub::DataReader<MotorControllerUnitModule::ControlMessage>(
    subscriber, controlTopic, readerQos);
  mMotorMessageReader = dds::sub::DataReader<MotorControllerUnitModule::MotorMessage>(
    subscriber, motorTopic, readerQos);

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

} // namespace dds_entities
