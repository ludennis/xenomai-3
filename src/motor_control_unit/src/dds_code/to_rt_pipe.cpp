#include <unistd.h>

#include <gen/carla_client_server_user_DCPS.hpp>

#include <dds_bridge.hpp>

#include <MessageTypes.h>
#include <RtMacro.h>

int main(int argc, char *argv[])
{
  char deviceName[] = "/dev/rtp0";
  auto fileDescriptor = open(deviceName , O_WRONLY | O_NONBLOCK);
  if (fileDescriptor < 0)
  {
    printf("file descriptor error opening %s\n", deviceName);
    return -1;
  }
  else
  {
    printf("File descriptor %s acquired\n", deviceName);
  }

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

  void *buffer = malloc(RtMessage::kMessageSize);
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

          // TODO: convert to xenomai MotorInputMessage
          MotorInputMessage motorInputMessage{1, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

          // and send it to pipe /dev/rtp0
          memcpy(buffer, &motorInputMessage, RtMessage::kMessageSize);
          auto bytesWritten = write(fileDescriptor, buffer, RtMessage::kMessageSize);
          if (bytesWritten <= 0)
          {
            printf("write error: %s\n", strerror(errno));
          }
          else
          {
            printf("Written %ld bytes into pipe", bytesWritten);
          }

        }
      }
    }
  }

  free(buffer);
  close(fileDescriptor);
  return 0;
}
