#include <RtDigitalOutputTask.h>

#include <RtMacro.h>

// TODO: make this a RT_xenomai periodic Task
int main(int argc, char* argv[])
{
  if (argc <= 2)
  {
    printf("Usage: do_main [PCI Bus Number] [PCI Device Number]\n");
    return -1;
  }

  RtDigitalOutputTask dioTask("RtDigitalOutputTask", RtMacro::kTaskStackSize,
    RtMacro::kMediumTaskPriority, RtMacro::kTaskMode, RtMacro::kOneSecondTaskPeriod,
    RtMacro::kCoreId7);

  dioTask.lineMaskPort0 = 0xFFFFFFFF; //use all lines on port 0
  dioTask.lineMaskPort1 = 0xFF; //use all lines on port 1
  dioTask.lineMaskPort2 = 0xFF; //use all lines on port 2
  dioTask.triStateOnExit = kTrue;

  dioTask.outputDataPort0 = 0x5A5A5A5A; // value to write to port0
  dioTask.outputDataPort1 = 0x99; // value to write to port1
  dioTask.outputDataPort2 = 0x66; // value to write to port2

  dioTask.port0Length = 0;
  dioTask.port1Length = 8;

  dioTask.lineMaskPFI =
    (dioTask.lineMaskPort2 << dioTask.port1Length) | dioTask.lineMaskPort1;
  dioTask.outputDataPFI =
    (dioTask.outputDataPort2 << dioTask.port1Length) | dioTask.outputDataPort1;
  dioTask.inputDataPort0 = 0x0; // value of lines on port 0
  dioTask.inputDataPFI = 0x0; //value of lines on port1:2 (PFI0..15)

  dioTask.Init(argv[1], argv[2]);
  dioTask.Write();

  return 0;
}
