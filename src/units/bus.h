
#ifndef RISCV_SIMULATOR_BUS_H
#define RISCV_SIMULATOR_BUS_H

#include <exception>
#include "../utils/config.h"
#include <utility>

class CommonDataBus {
  friend class Register;
private:
  struct BusEntry {
    bool busy;
    int label; // for ST calls, only need label
    int value;
    int rd = -1; // only used in commit_bus
  };
public:
  void PutOnBus(int label, int value, int rd = -1) {
    int index = 0;
    for (int i = 0; i < CDBSIZE; ++i) {
      if (bus[i].busy) index = i + 1;
      else break;
    }
    if (index == CDBSIZE) throw std::exception();
    bus[index] = {true, label, value, rd};
  }

  std::pair<bool, int> TryGetValue(int label) const {
    for (int i = 0; i < CDBSIZE; ++i) {
      if (bus[i].busy && bus[i].label == label) {
        return {true, bus[i].value};
      }
    }
    return {false, 0};
  }

  void clear() {
    for (int i = 0; i < CDBSIZE; ++i) {
      bus[i].busy = false;
    }
  }

private:
  BusEntry bus[CDBSIZE];
};

#endif //RISCV_SIMULATOR_BUS_H
