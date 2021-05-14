#include <memory>

#include <PxiCardTask.h>

int main(int argc, char **argv)
{
  auto pxiCardTask = std::make_shared<PxiCardTask>();

  pxiCardTask->ListAllCards();

  return 0;
}
