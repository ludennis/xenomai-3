#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <string>

#include <alchemy/timer.h>

#include <gen/carla_client_server_user_DCPS.hpp>

#include <dds_bridge.hpp>

#include <MessageTypes.h>
#include <RtMacro.h>

int fileDescriptor;
void *buffer;

void TerminationHandler(int signal)
{
  printf("Termination signal received. Exiting ...\n");
  free(buffer);
  close(fileDescriptor);
  exit(1);
}

int main(int argc, char *argv[])
{
  struct sigaction action;
  action.sa_handler = TerminationHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  char deviceName[] = "/dev/rtp0";
  fileDescriptor = open(deviceName , O_WRONLY | O_NONBLOCK);
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

  buffer = malloc(RtMessage::kMessageSize);
  for (;;)
  {
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

          // TODO: or check validity of message
          if (sampleData.simulation_time().empty())
          {
            continue;
          }

          printf("sample.id = %d, sample.throttle = %f, sample.vehicle_speed = %f, "
            "sample.simulation_time = %s\n", sampleData.id(), sampleData.throttle(),
            sampleData.vehicle_speed(), sampleData.simulation_time().c_str());


          auto timeNow = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

          printf("DDS Message travel time: %ld ns\n",
            timeNow - std::stoul(sampleData.simulation_time()));

          // TODO: convert to xenomai MotorInputMessage
          MotorInputMessage motorInputMessage{1, rt_timer_read(), 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

          // and send it to pipe /dev/rtp0
          memcpy(buffer, &motorInputMessage, RtMessage::kMessageSize);
          printf("memcpy ed\n");
          auto bytesWritten = write(fileDescriptor, buffer, RtMessage::kMessageSize);
          printf("Written %ld bytes\n", bytesWritten);
          // TODO: fix weird behavior of not going into the following if statements
          // and also receiving an empty message when throttle_emulator quits
        }
      }
    }
  }

  return 0;
}
