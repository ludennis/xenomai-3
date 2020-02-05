#include <chrono>
#include <thread>

#include <dds/dds.hpp>

#include <ControlMessage_DCPS.hpp>

int main()
{
  // domain participant
  dds::domain::DomainParticipant participant(1);

  // topic
  dds::topic::Topic<ControlMessage> topic(participant, "control_topic");

  // publisher
  dds::pub::Publisher publisher(participant);

  // data writer
  dds::pub::DataWriter<ControlMessage> writer(publisher, topic);


  // data reader

  return 0;
}