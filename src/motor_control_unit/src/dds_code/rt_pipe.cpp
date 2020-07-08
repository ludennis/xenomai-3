#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <thread>

#include <MessageTypes.h>
#include <RtMacro.h>

int main(int argc, char *argv[])
{
  auto fileDescriptor = open("/dev/rtp1", O_RDONLY);

  if (fileDescriptor < 0)
    printf("file descriptor error: %s\n", strerror(errno));
  else
    printf("file descriptor acquired\n");

  for (;;)
  {
    MotorMessage *motorMessage = (MotorMessage*) malloc(RtMessage::kMessageSize);
    auto bytesRead = read(fileDescriptor, motorMessage, RtMessage::kMessageSize);
    printf("Read bytes %ld from fileDescriptor\n", bytesRead);
    if (bytesRead > 0)
    {
      printf("motorMessage rpm: %f\n", motorMessage->ft_RotorRPM);
    }
    if (bytesRead < 0)
      printf("read error: %s\n", strerror(errno));
    free(motorMessage);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  close(fileDescriptor);

  return 0;
}
