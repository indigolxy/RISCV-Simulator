
#ifndef RISCV_SIMULATOR_STACK_H
#define RISCV_SIMULATOR_STACK_H

template <typename T, int size>
class Stack {
public:
  Stack() = default;

  void push(const T &src) {
    if (sp == size) {
      for (int i = 0; i < size - 1; ++i) {
        data[i] = data[i + 1];
      }
      data[size - 1] = src;
    }
    else {
      data[sp++] = src;
    }
  }

  T pop() {
    if (sp == 0) return T();
    return data[--sp];
  }

private:
  T data[size];
  int sp = 0;
};

#endif //RISCV_SIMULATOR_STACK_H
