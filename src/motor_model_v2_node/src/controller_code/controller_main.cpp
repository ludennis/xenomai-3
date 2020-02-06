#include <thread>

#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

int main()
{
  utils::ElapsedTimes roundTripElapsedTimes;

  // domain participant
  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());

  // topic
  dds::topic::Topic<MotorControllerUnitModule::ControlMessage> contorlTopic(participant, "control_topic");

  // publisher
  dds::pub::Publisher publisher(participant);

  // data writer
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage>
    controlMessageWriter(publisher,contorlTopic);

  // data reader with waitset
  dds::sub::Subscriber subscriber(participant);
  dds::topic::Topic<MotorControllerUnitModule::MotorMessage>
    motorTopic(participant, "motor_topic");
  dds::sub::DataReader<MotorControllerUnitModule::MotorMessage>
    reader(subscriber, motorTopic);
  dds::core::cond::WaitSet waitSet;
  dds::core::cond::StatusCondition dataAvailableCondition(reader);
  dataAvailableCondition.enabled_statuses(
    dds::core::status::StatusMask::data_available());
  waitSet.attach_condition(dataAvailableCondition);

  dds::core::cond::WaitSet::ConditionSeq conditionSeq;
  dds::core::Duration waitSetConnectionTimeout(5, 0); // waits for 25 seconds
  dds::core::Duration waitSetTransmissionTimeout(1, 0); // waits for 1 second

  // establish connection before transmission
  std::cout << "Connecting to motor ...";
  for(auto count{1ll};;++count)
  {
    controlMessageWriter <<
      MotorControllerUnitModule::ControlMessage(count, "connect");
    try
    {
      waitSet.wait(conditionSeq, waitSetConnectionTimeout);
      reader.take();
      std::cout << "Connected!" << std::endl;
      break;
    }
    catch(dds::core::TimeoutError &e)
    {
      (void)e;
    }
  }

  for(auto count{1ll};;++count)
  {
    // send control signals to motor
    auto beginRoundTripTime = std::chrono::high_resolution_clock::now();
    controlMessageWriter << MotorControllerUnitModule::ControlMessage(1, "motor_step");
    std::cout << "requesting motor to step ... ";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    waitSet.wait(conditionSeq, waitSetTransmissionTimeout);
    auto samples = reader.take();
    for(auto sample = samples.begin(); sample != samples.end(); ++sample)
    {
      if(sample->info().valid())
      {
        std::cout << sample->data().content() << std::endl;
      }
    }

    auto endRoundTripTime = std::chrono::high_resolution_clock::now();
    roundTripElapsedTimes.AddTime(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        endRoundTripTime - beginRoundTripTime));

    roundTripElapsedTimes.PrintHeader("controller");
    roundTripElapsedTimes.Print("round trip");
  }

  return 0;
}