
#ifndef RISCV_SIMULATOR_PREDICTOR_H
#define RISCV_SIMULATOR_PREDICTOR_H

#include "../utils/stack.h"
#include "../utils/config.h"

class Predictor {
public:
  Predictor() = default;

  bool BJump(u32 code) {
    // todo
    return true;
  }

  void AddJalAdd(int addr) {
    jal_stack.push(addr);
  }

  int JALRJump() {
    return jal_stack.pop();
  }

private:
  Stack<int, PREDICT_STACK_SIZE> jal_stack;
};

#endif //RISCV_SIMULATOR_PREDICTOR_H
