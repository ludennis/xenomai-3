#include <DigitalInputOutputTask.h>

#include <memory>

int main(int argc, char *argv[])
{
  if (argc <= 2)
  {
    printf("Usage: di_main [Bus Number] [Device Number]\n");
    return -1;
  }

  auto diTask = std::make_unique<DigitalInputOutputTask>();

  diTask->lineMaskPort0 = 0xFFFFFFFF;
  diTask->lineMaskPort1 = 0xFF;
  diTask->lineMaskPort2 = 0xFF;
  diTask->triStateOnExit = kTrue;

  diTask->port0Length = 0;
  diTask->port1Length = 0;

  diTask->lineMaskPFI =
    (diTask->lineMaskPort2 << diTask->port1Length) | diTask->lineMaskPort1;
  diTask->inputDataPort0 = 0x0;
  diTask->inputDataPFI = 0x0;

  diTask->Init(argv[1], argv[2]);
  diTask->Read();
  return 0;
}
