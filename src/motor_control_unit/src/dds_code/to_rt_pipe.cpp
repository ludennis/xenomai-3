#include <gen/carla_client_server_user_DCPS.hpp>

#include <dds_bridge.hpp>

int main(int argc, char *argv[])
{
  dds_bridge::DDSBridge ddsBridge;

  ddsBridge.CreateDomainParticipant();
  ddsBridge.CreateSubscriber();

  auto readerQos = ddsBridge.CreateDataReaderQos();
  readerQos << dds::core::policy::Reliability::Reliable();
  auto reader =
    ddsBridge.CreateDataReader<basic::module_vehicleSignal::vehicleSignalStruct>(
      "VehicleSignalTopic", readerQos);

  ddsBridge.CreateWaitSet();
  auto statusCondition = ddsBridge.CreateStatusCondition(reader);
  ddsBridge.EnableStatus(statusCondition, dds::core::status::StatusMask::data_available());
  ddsBridge.AddStatusCondition(statusCondition);

  for (;;)
  {
    printf("waiting for messages ...\n");
    auto conditions = ddsBridge.mWaitSet.wait();
    for (auto i{0u}; i < conditions.size(); ++i)
    {
      if (conditions[i] == statusCondition)
      {
        auto samples = reader.take();

        for(auto itr{samples.begin()}; itr != samples.end(); ++itr)
        {
          const auto &sample = *itr;
          const auto sampleData = sample.data();
          printf("sample.id = %d, sample.throttle = %f, sample.vehicle_speed = %f\n",
            sampleData.id(), sampleData.throttle(), sampleData.vehicle_speed());
        }
      }
    }
  }

  return 0;
}
