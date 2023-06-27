#include <iostream>
#include "cpu.h"

int main () {
  CPU cpu;
  cpu.Init();
  std::cout << cpu.run();
  return 0;
}
