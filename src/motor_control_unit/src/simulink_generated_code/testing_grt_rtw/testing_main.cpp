#include <iostream>
#include <memory>

#include <testing.h>

#include <RtTaskHandler.h>

static auto rtTaskHandler = std::make_unique<RtTaskHandler>();

int main(int argc, char **argv)
{
  DWORD cardNum = 3;
  rtTaskHandler->OpenCard(cardNum);

  auto testingModel = testingModelClass();
  testingModel.initialize();
  for(auto i{0u}; i < 10u; ++i)
  {
    testingModel.step();
    std::cout << "Out1 after step(): " << testingModel.testing_Y.Out1 << std::endl;
  }

  testingModel.terminate();

  return 0;
}
