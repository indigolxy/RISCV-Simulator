
#ifndef RISCV_SIMULATOR_REGISTER_H
#define RISCV_SIMULATOR_REGISTER_H

#include "../utils/config.h"
#include <utility>

class Register {
public:
  Register() = default;

  void flush() {
    for (int i = 0; i < REGNUM; ++i) {
      reg_now[i] = reg_next[i];
    }
  }

  std::pair<int, int> GetValueDependency(int num) const {
    return {reg_now[num].data, reg_now[num].dependency};
  }

  void SetDependency(int num, int label) {
    reg_next[num].dependency = label;
  }

private:
  struct RegisterEntry {
    int data = 0;
    int dependency = -1;
  };
  RegisterEntry reg_now[REGNUM];
  RegisterEntry reg_next[REGNUM];

};

#endif //RISCV_SIMULATOR_REGISTER_H
