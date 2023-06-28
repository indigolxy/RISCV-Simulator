
#ifndef RISCV_SIMULATOR_RSS_H
#define RISCV_SIMULATOR_RSS_H

#include "instuction.h"
#include "../utils/config.h"
#include "register.h"
#include "alu.h"
#include "bus.h"
#include "../storage/lsb.h"

class ReservationStation {
private:
  struct RssEntry {
    OptType opt;
    int value1 = 0, value2 = 0;
    int dependency1 = -1, dependency2 = -1;
    int label = 0; // in RoB
    int imm = 0;
  };
public:
  ReservationStation() = default;

  void flush() {
    for (int i = 0; i < RSSSIZE; ++i) {
      rss_now[i] = rss_next[i];
    }
    size_now = size_next;
  }

  bool full() const {return size_now == RSSSIZE;}

  void Clear() {size_next = 0;}

  /*
   * read values and dependency in reg
   * add an entry
   */
  void issue(int rob_index, const InstructionUnit::Instruction &ins, const Register &reg);

  /*
   * find an entry without dependency and calculate in ALU and get result
   * put the information into bus(label, value)
   * remove entry
   */
  void AriExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, int pc);

  /*
   * find an entry without dependency
   * if a ST is at top and without dependency,
   *     calculate the addr and value, pop it into lsb and remove entry
   * if a LD is without dependency and has no STs before it,
   *     calculate its addr, pop it into lsb(and then lsb.execute) and remove entry
   */
  void LsExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, LoadStoreBuffer &lsb);

  /*
   * monitor bus and clear dependency(check dependency)
   */
  void CheckBus(const CommonDataBus &cdb);

private:
  // 从下标0开始放，到size - 1为止
  RssEntry rss_now[RSSSIZE];
  int size_now;
  RssEntry rss_next[RSSSIZE];
  int size_next;

  int FindIndependentEntry() {
    for (int i = 0; i < size_now; ++i) {
      if (rss_now[i].dependency1 == -1 && rss_now[i].dependency2 == -1)
        return i;
    }
    return -1;
  }

  void RemoveEntry(int index) {
    --size_next;
    for (int i = index; i < size_next; ++i) {
      rss_next[i] = rss_next[i + 1];
    }
  }

};

#endif //RISCV_SIMULATOR_RSS_H
