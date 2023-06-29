
#ifndef RISCV_SIMULATOR_REGISTER_H
#define RISCV_SIMULATOR_REGISTER_H

#include "../utils/config.h"
#include "../units/bus.h"
#include <utility>

class Register {
public:
  Register() = default;

  void FlushSetX0() {
    for (int i = 0; i < REGNUM; ++i) {
      reg_now[i] = reg_next[i];
    }
    reg_now[0].data = 0;
    reg_now[0].dependency = -1;
  }

  std::pair<int, int> GetValueDependency(int num) const {
    return {reg_now[num].data, reg_now[num].dependency};
  }

  void SetDependency(int num, int label) {
    reg_next[num].dependency = label;
  }

  void ClearDependency() {
    for (int i = 0; i < REGNUM; ++i) {
      reg_next[i].dependency = -1;
    }
  }

  u8 GetRet() const {
    return (u32(reg_now[10].data)) & 255u;
  }

  /*
   * for all entrys in cdb, write data in x[rd]
   * if x[rd]'s dependency == label in cdb, clear dependency
   */
  void CheckBus(const CommonDataBus &cdb) {
    for (int i = 0; i < CDBSIZE; ++i) {
      if (cdb.bus[i].busy && cdb.bus[i].rd > 0) {
        reg_next[cdb.bus[i].rd].data = cdb.bus[i].value;
        if (reg_next[cdb.bus[i].rd].dependency == cdb.bus[i].label)
          reg_next[cdb.bus[i].rd].dependency = -1;
      }
    }
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
