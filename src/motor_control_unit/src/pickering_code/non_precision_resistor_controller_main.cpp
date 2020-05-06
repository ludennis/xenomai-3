#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <Pilpxi.h>

int main(int argc, char **argv)
{
  DWORD numOfFreeCards;
  DWORD buses[100]; // assume theres a maximum of 100 cards
  DWORD devices[100]; // assume theres a maximum of 100 cards
  DWORD bus;
  DWORD device;
  DWORD cardNum;
  DWORD err;
  DWORD resistance;
  CHAR id[100];
  DWORD data[100];

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
  else if(argc == 4)
  {
    bus = std::atoi(argv[1]);
    device = std::atoi(argv[2]);
    resistance = std::atoi(argv[3]);

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
        PIL_ReadSub(cardNum, 1, data);

        printf("Resistance = %d\n", data[0]);

        data[0] = resistance;

        // write sub with data read from sub unit
        printf("writing sub\n");
        PIL_WriteSub(cardNum, 1, data);
        printf("Resistance changed to = %d\n", data[0]);
      }
      // close card
      PIL_CloseSpecifiedCard(cardNum);
    }
    else
    {
      printf("error return is %d\n", err);
    }
  }
  else
  {
    printf("usage: non_precision_resistor_controller [bus] [device] [resistance]\n");
  }

  return 0;
}
