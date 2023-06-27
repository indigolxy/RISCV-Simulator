
#ifndef RISCV_SIMULATOR_BUS_H
#define RISCV_SIMULATOR_BUS_H

#include <exception>
#include "../utils/config.h"

class CommonDataBus {
private:
  struct BusEntry {
    bool busy;
    int label;
    int value;
    int rd = 0;
  };
public:
  void PutOnBus(int label, int value) {
    int index = 0;
    for (int i = 0; i < CDBSIZE; ++i) {
      if (bus[i].busy) index = i + 1;
      else break;
    }
    if (index == CDBSIZE) throw std::exception();
    bus[index] = {true, label, value, 0};
  }

private:
  BusEntry bus[CDBSIZE];
};

#endif //RISCV_SIMULATOR_BUS_H
