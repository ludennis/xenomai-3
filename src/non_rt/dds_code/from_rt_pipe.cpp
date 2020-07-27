#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include <MessageTypes.h>
#include <RtMacro.h>
#include <dds_bridge.hpp>
#include <gen/carla_client_server_user_DCPS.hpp>

int main(int argc, char *argv[])
{
  auto fileDescriptor = open("/dev/rtp1", O_RDONLY);

  if (fileDescriptor < 0)
    printf("file descriptor error: %s\n", strerror(errno));
  else
    printf("file descriptor acquired\n");

  // DDS
  dds_bridge::DDSBridge ddsBridge;
  ddsBridge.CreateDomainParticipant();
  ddsBridge.CreatePublisher();

  auto writerQos = ddsBridge.CreateDataWriterQos();
  writerQos << dds::core::policy::Reliability::Reliable();
  auto writer =
    ddsBridge.CreateDataWriter<basic::module_vehicleSignal::vehicleSignalStruct>(
      "VehicleSignalTopic", writerQos);

  auto numMessage{0u};
  for (;;)
  {
    MotorOutputMessage motorOutputMessage;
    auto bytesRead = read(fileDescriptor, &motorOutputMessage, sizeof(MotorOutputMessage));
    printf("Read bytes %ld from fileDescriptor\n", bytesRead);
    if (bytesRead > 0)
    {
      printf("motorOutputMessage rpm: %f\n", motorOutputMessage.ft_RotorRPM);

      // write dds message
      basic::module_vehicleSignal::vehicleSignalStruct vehicleSignalMessage;
      vehicleSignalMessage.id(numMessage++);
      vehicleSignalMessage.vehicle_speed(
        motorOutputMessage.ft_RotorRPM / motorOutputMessage.ft_OutputTorque);
      vehicleSignalMessage.throttle(motorOutputMessage.ft_RotorRPM);
      writer.write(vehicleSignalMessage);
    }
    if (bytesRead < 0)
      printf("read error: %s\n", strerror(errno));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  close(fileDescriptor);

  return 0;
}
