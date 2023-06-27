
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

  /*
   * start from addr
   * fetch 4 continuous bytes and put together reversely
   */
  u32 FetchWordReverse(int addr) {
    u32 tmp = 0;
    u32 ret = 0;
    for (int i = 0; i < 4; ++i) {
      tmp = u32(units[addr + i]);
      tmp = tmp << (i * 8);
      ret = ret | tmp;
    }
    return ret;
  }

private:
  u8 units[MEMSIZE];
};

#endif //RISCV_SIMULATOR_MEMORY_H
