#include <chrono>
#include <thread>

#include <dds/dds.hpp>

#include <MotorControllerUnitModule_DCPS.hpp>

int main()
{
  // domain participant
  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());

  // topic
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage> contorlTopic(participant, "control_topic");

  // publisher
  dds::pub::Publisher publisher(participant);

  // data writer
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage> writer(publisher,contorlTopic);

  for(auto count{1ll};;++count)
  {
    // send control signals to motor
    writer << MotorControllerUnitModule::ControlMessage(1, "motor_step");
    std::cout << "requesting motor to step ... " << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // data reader

  return 0;
}