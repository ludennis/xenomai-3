#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <string>

#include <alchemy/timer.h>

#include <dds_bridge.hpp>
#include <gen/carla_client_server_user_DCPS.hpp>
#include <MessageTypes.h>
#include <RtMacro.h>

void *buffer;
int fileDescriptor;

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
  auto dataAvailableCondition = ddsBridge.CreateStatusCondition(reader);
  ddsBridge.EnableStatus(
    dataAvailableCondition, dds::core::status::StatusMask::data_available());
  ddsBridge.AddStatusCondition(dataAvailableCondition);

  buffer = malloc(RtMessage::kMessageSize);
  dds::core::cond::WaitSet::ConditionSeq conditions;
  for (;;)
  {
    try
    {
      conditions = ddsBridge.mWaitSet.wait(dds::core::Duration(1));
    }
    catch (const dds::core::TimeoutError& timeoutException)
    {
      continue;
    }

    for (auto i{0u}; i < conditions.size(); ++i)
    {
      if (conditions[i] == dataAvailableCondition)
      {
        auto samples = reader.take();

        for(auto itr{samples.begin()}; itr != samples.end(); ++itr)
        {
          const auto &sample = *itr;
          const auto sampleData = sample.data();

          // skip bad messages
          // TODO: check more details for message validity
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

          // rt_timer_read() for recording RTIME to be measured by motor
          MotorInputMessage motorInputMessage{1, rt_timer_read(), 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

          memcpy(buffer, &motorInputMessage, RtMessage::kMessageSize);
          auto bytesWritten = write(fileDescriptor, buffer, RtMessage::kMessageSize);
          printf("Written %ld bytes to %s\n", bytesWritten, deviceName);
        }
      }
    }
  }

  return 0;
}
