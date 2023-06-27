
#ifndef RISCV_SIMULATOR_ALU_H
#define RISCV_SIMULATOR_ALU_H

class ArithmeticLogicUnit {
public:
  int ADD(int a, int b) const {
    return a + b;
  }

  bool IsEqual(int a, int b) const {
    return a == b;
  }
  bool IsLessThanSigned(int a, int b) const {
    return a < b;
  }
  bool IsLessThanUnsigned(u32 a, u32 b) const {
    return a < b;
  }

  int AND(int a, int b) const {
    return a & b;
  }
  int XOR(int a, int b) const {
    return a ^ b;
  }
  int OR(int a, int b) const {
    return a | b;
  }

  int ShiftLeftLogical(int a, int shamt) const {
    return a << shamt;
  }
  int ShiftRightLogical(int a, int shamt) const {
    return a >> shamt;
  }
  int ShiftRightAri(int a, int shamt) const {
    if (a >= 0) return a >> shamt;
    u32 tmp = 0;
    for (int i = 0; i < shamt; ++i) {
      tmp = tmp | (1 << (31 - i));
    }
    return int(tmp) | (a >> shamt);
  }
};

#endif //RISCV_SIMULATOR_ALU_H
