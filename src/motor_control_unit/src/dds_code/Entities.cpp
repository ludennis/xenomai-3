#include <Entities.h>

namespace dds_entities
{

Entities::Entities()
  : mParticipant(dds::core::null)
  , mPublisher(dds::core::null)
  , mSubscriber(dds::core::null)
  , mWaitSet(dds::core::null)
{}

void Entities::CreateDomainParticipant()
{
  this->mParticipant = dds::domain::DomainParticipant(org::opensplice::domain::default_id());
}

void Entities::CreateDomainParticipant(unsigned int id)
{
  this->mParticipant = dds::domain::DomainParticipant(id);
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

dds::pub::qos::DataWriterQos Entities::CreateDataWriterQos()
{
  dds::pub::qos::DataWriterQos writerQos;
  return writerQos;
}

dds::sub::qos::DataReaderQos Entities::CreateDataReaderQos()
{
  dds::sub::qos::DataReaderQos readerQos;
  return readerQos;
}

void Entities::CreateWaitSet()
{
  this->mWaitSet = dds::core::cond::WaitSet();
}

void Entities::AddStatusCondition(dds::core::cond::StatusCondition &statusCondition)
{
  this->mWaitSet += statusCondition;
}

void Entities::AddGuardCondition(dds::core::cond::GuardCondition &guardCondition)
{
  this->mWaitSet += guardCondition;
}

} // namespace dds_entities
