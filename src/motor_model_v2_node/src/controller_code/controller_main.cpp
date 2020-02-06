#include <thread>

#include <dds/dds.hpp>

#include <utils/ElapsedTimes.hpp>
#include <idl/gen/MotorControllerUnitModule_DCPS.hpp>

int main()
{
  utils::ElapsedTimes roundTripElapsedTimes;

  dds::domain::DomainParticipant participant(org::opensplice::domain::default_id());

  dds::topic::Topic<MotorControllerUnitModule::ControlMessage>
    contorlTopic(participant, "control_topic");
  dds::topic::Topic<MotorControllerUnitModule::MotorMessage>
    motorTopic(participant,"motor_topic");

  dds::sub::Subscriber subscriber(participant);
  dds::sub::DataReader<MotorControllerUnitModule::MotorMessage>
    reader(subscriber,motorTopic);

  dds::pub::Publisher publisher(participant);
  dds::pub::DataWriter<MotorControllerUnitModule::ControlMessage>
    controlMessageWriter(publisher,contorlTopic);

  dds::core::cond::WaitSet waitSet;
  dds::core::cond::StatusCondition dataAvailableCondition(reader);
  dataAvailableCondition.enabled_statuses(
    dds::core::status::StatusMask::data_available());
  waitSet.attach_condition(dataAvailableCondition);
  dds::core::cond::WaitSet::ConditionSeq conditionSeq;
  dds::core::Duration waitSetConnectionTimeout(5, 0);
  dds::core::Duration waitSetTransmissionTimeout(1, 0);

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
    auto beginRoundTripTime = std::chrono::high_resolution_clock::now();
    controlMessageWriter << MotorControllerUnitModule::ControlMessage(1, "motor_step");
    std::cout << "requesting motor to step ... ";

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