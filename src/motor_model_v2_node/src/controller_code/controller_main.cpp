#include <chrono>
#include <thread>

#include <dds/dds.hpp>

#include <MotorControllerUnitModule_DCPS.hpp>

int main()
{
  // domain participant
  dds::domain::DomainParticipant participant(1);

  // topic
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage> contorlTopic(participant, "control_topic");

  // publisher
  dds::pub::Publisher publisher(participant);

  // data writer
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage> writer(publisher,contorlTopic);

  for(auto count{1ll};;++count)
  {
    // send control signals to motor
    writer << MotorControllerUnitModule::ControlMessage(1, "ping");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // data reader

  return 0;
}