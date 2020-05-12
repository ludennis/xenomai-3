#include <iostream>

#include <testing.h>

int main(int argc, char **argv)
{
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
