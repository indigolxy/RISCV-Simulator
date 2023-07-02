
#ifndef RISCV_SIMULATOR_MEMORY_H
#define RISCV_SIMULATOR_MEMORY_H

#include <cstring>
#include <iostream>
#include <iomanip>
#include "../utils/config.h"

class Memory {
public:
  Memory() {
    memset(units, 0, sizeof (units));
  }

  static int SignExtend(u32 src, int len) {
    u32 tmp = src >> (len - 1);
    if (tmp == 0) return int(src);
    tmp = 0;
    for (int i = 0; i < len; ++i) {
      tmp = tmp | (1 << i);
    }
    tmp = ~tmp;
    tmp = tmp | src;
    return int(tmp);
  }

  static int GetByte(int src) {
    return src & 0xff;
  }

  static int GetHalf(int src) {
    return src & 0xffff;
  }

  static int GetHighByte(int src) {
    src = src & 0xff00;
    src = src >> 8;
    return src;
  }

  static int GetHighHalf(int src) {
    src = src & 0xffff0000;
    src = src >> 16;
    return src;
  }

  static int GetMidHalf(int src) {
    src = src & 0xffff00;
    src = src >> 8;
    return src;
  }

  void StoreByte(int addr, int value) {
    units[addr] = u8(value);
  }

  void StoreHalf(int addr, int value) {
    units[addr + 1] = u8(GetHighByte(value));
    units[addr] = u8(GetByte(value));
  }

  void StoreWord(int addr, int value) {
    units[addr + 3] = u8(GetHighByte(GetHighHalf(value)));
    units[addr + 2] = u8(GetByte(GetHighHalf(value)));
    units[addr + 1] = u8(GetHighByte(value));
    units[addr] = u8(GetByte(value));
  }

  u32 LoadByte(int addr) const {
    return u32(units[addr]);
  }

  u32 LoadHalf(int addr) const {
    u32 tmp1 = (u32)units[addr + 1] << 8;
    u32 tmp2 = (u32)units[addr];
    return tmp1 | tmp2;
  }

  u32 LoadWord(int addr) const {
    u32 tmp1 = (u32)units[addr + 3] << 24;
    u32 tmp2 = (u32)units[addr + 2] << 16;
    u32 tmp3 = (u32)units[addr + 1] << 8;
    u32 tmp4 = (u32)units[addr];
    return tmp1 | tmp2 | tmp3 | tmp4;
  }

  /*
   * read instructions and put into memory
   * return PC value
   */
  int InitInstructions() {
    int addr = 0, ret = 0;
    getchar();
    std::cin >> std::hex >> addr;
    ret = addr;
    while (!std::cin.eof()) {
      while (!std::cin.eof()) {
        int tmp;
        std::cin >> std::hex >> tmp;
        units[addr] = u8(tmp);
        getchar();
        if (std::cin.peek() == '\n') getchar();
        if (std::cin.peek() == '@') break;
        ++addr;
      }
      getchar();
      std::cin >> std::hex >> addr;
    }
    return ret;
  }

private:
  u8 units[MEMSIZE];
};

#endif //RISCV_SIMULATOR_MEMORY_H
