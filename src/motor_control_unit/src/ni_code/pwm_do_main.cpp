#include <memory>

#include <DigitalInputOutputTask.h>

int main(int argc, char *argv[])
{

  auto doPwmTask = std::make_unique<DigitalInputOutputTask>();

  if (argc <= 2)
  {
    printf("Usage: pwm_do_main.cpp [bus number] [device number]\n");
    exit(-1);
  }

  // set pfi DO
  doPwmTask->Init(argv[1], argv[2]);

  // set PWM registers
  // pfiDioHelper for masking the pfi lines
  doPwmTask->ConfigurePfiDioLines(0x00000001);

  // set timers?
  const unsigned int samplePeriod = 100000;
  const nOutTimer::tOutTimer_Continuous_t operationType = nOutTimer::kContinuousOp;
  // start

  return 0;
}
