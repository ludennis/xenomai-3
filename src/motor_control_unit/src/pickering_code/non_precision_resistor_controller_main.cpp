#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <alchemy/task.h>

#include <Pilpxi.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 1000; // 1 us
constexpr auto kNanosecondsToMicroseconds = 1e3;
constexpr auto kNanosecondsToMilliseconds = 1e6;
constexpr auto kNanosecondsToSeconds = 1e9;

static RT_TASK rtTask;

static DWORD numOfFreeCards;
static DWORD buses[100]; // assume theres a maximum of 100 cards
static DWORD devices[100]; // assume theres a maximum of 100 cards
static DWORD resistance;
static CHAR id[100];
static DWORD data[100];
static DWORD subunit;
static DWORD bus;
static DWORD device;
static DWORD err;
static DWORD cardNum;

void SetSubunitResistanceRoutine(void*)
{
  printf("Accessing bus %d, device %d, target resistance %d\n", bus, device, resistance);

  // check target id is a resistor
  err = PIL_OpenSpecifiedCard(bus, device, &cardNum);

  if(err == 0)
  {
    printf("Card Number is %d\n", cardNum);
    PIL_CardId(cardNum, id);
    printf("Card id is %s\n", id);

    // check if the card is a resistor
    std::string cardId(id);
    std::string resistorId("40-295-121");
    if(cardId.find(resistorId) != std::string::npos)
    {
      printf("it has resistor id(%s)!\n", resistorId.c_str());
    }
    else
    {
      printf("Card doesn't have resistor id (%s), might not be a resistor card\n",
        resistorId.c_str());
    }

    // change the resistance

    if(resistance > 0.0)
    {
      // read sub to get data
      printf("reading sub\n");
      PIL_ReadSub(cardNum, subunit, data);

      printf("Resistance = %d\n", data[0]);

      data[0] = resistance;

      // write sub with data read from sub unit
      printf("writing sub\n");
      PIL_WriteSub(cardNum, subunit, data);
      printf("Resistance changed to = %d\n", data[0]);
    }
    PIL_ClearSub(cardNum, subunit);
    // close card
    PIL_CloseSpecifiedCard(cardNum);
  }
  else
  {
    printf("error return is %d\n", err);
  }

}

int main(int argc, char **argv)
{
  if(argc == 1)
  {
    // find free cards
    PIL_CountFreeCards(&numOfFreeCards);
    printf("Found %d free cards\n", numOfFreeCards);
    PIL_FindFreeCards(numOfFreeCards, buses, devices);
    printf("Card addresses:\n");
    for(auto i{0}; i < numOfFreeCards; ++i)
    {
      printf("Opening card at bus %d, device %d\n", buses[i], devices[i]);
      err = PIL_OpenSpecifiedCard(buses[i], devices[i], &cardNum);
      if(err == 0)
      {
        printf("Card Number is %d\n", cardNum);
        PIL_CardId(cardNum, id);
        printf(" Card id is %s\n", id);
        PIL_CloseSpecifiedCard(cardNum);
      }
      else
      {
        printf("Error return is %d\n", err);
      }
    }
  }
  else if(argc == 5)
  {
    bus = std::atoi(argv[1]);
    device = std::atoi(argv[2]);
    subunit = std::atoi(argv[3]);
    resistance = std::atoi(argv[4]);

    // make it an rt task
    int e1 = rt_task_create(&rtTask, "SetSubunitResistanceRoutine",
      kTaskStackSize, kMediumTaskPriority, kTaskMode);
    int e2 = rt_task_set_periodic(&rtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
    int e3 = rt_task_start(&rtTask, &SetSubunitResistanceRoutine, NULL);
//    SetSubunitResistance();
  }
  else
  {
    printf("usage: non_precision_resistor_controller [bus] [device] [subunit] [resistance]\n");
  }

  return 0;
}
