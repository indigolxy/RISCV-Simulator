
#ifndef RISCV_SIMULATOR_PREDICTOR_H
#define RISCV_SIMULATOR_PREDICTOR_H

#include <bitset>
#include "../utils/stack.h"
#include "../utils/config.h"

class Predictor {
public:
  Predictor() = default;

  bool BJump(int pc) {
    int tmp = pc % PREDICT_COUNTER_NUM;
    if (counter[tmp].test(1)) return true;
    return false;
  }

  void SetJump(int pc, bool jump) {
    int tmp = pc % PREDICT_COUNTER_NUM;
    if (jump) {
      if (counter[tmp].test(1)) {
        counter[tmp].set(0);
      }
      else {
        if (!counter[tmp].test(0)) {
          counter[tmp].set(0);
        }
        else {
          counter[tmp].set(1);
          counter[tmp].reset(0);
        }
      }
    }
    else {
      if (counter[tmp].test(1)) {
        if (counter[tmp].test(0)) {
          counter[tmp].reset(0);
        }
        else {
          counter[tmp].set(0);
          counter[tmp].reset(1);
        }
      }
      else {
        counter[tmp].reset(0);
      }
    }
  }

  void AddJalAdd(int addr) {
    jal_stack.push(addr);
  }

  int JALRJump() {
    if (jal_stack.empty()) return -1;
    return jal_stack.pop();
  }

private:
  Stack<int, PREDICT_STACK_SIZE> jal_stack;
  std::bitset<2> counter[PREDICT_COUNTER_NUM] = {0};
};

#endif //RISCV_SIMULATOR_PREDICTOR_H
