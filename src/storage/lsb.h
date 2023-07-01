
#ifndef RISCV_SIMULATOR_LSB_H
#define RISCV_SIMULATOR_LSB_H

#include "../utils/circular_queue.h"
#include "../utils/config.h"
#include "../units/instuction.h"
#include "../storage/memory.h"
#include "../units/bus.h"

class LoadStoreBuffer {
private:
  struct LsbEntry {
    int cnt = -1;
    bool ready;
    OptType opt;
    int addr = -1;
    int value = -1;
    int label = -1;

    friend std::ostream &operator<<(std::ostream &os, const LoadStoreBuffer::LsbEntry &obj) {
      os << "label = " << obj.label << ", opt = ";
      switch (obj.opt) {
        case OptType::LB : os << "LB"; break;
        case OptType::LH : os << "LH"; break;
        case OptType::LW : os << "LW"; break;
        case OptType::LBU : os << "LBU"; break;
        case OptType::LHU : os << "LHU"; break;
        case OptType::SB : os << "SB"; break;
        case OptType::SH : os << "SH"; break;
        case OptType::SW : os << "SW"; break;
      }
      os << ", addr = " << std::hex << obj.addr << ", value = " << obj.value << std::dec;
      if (obj.ready) os << ", is ready.";
      else os << ", is not ready.";
      return os;
    }
  };

public:
  LoadStoreBuffer() = default;

  void print();

  void flush();

  /*
   * all ready ST shouldn't be cleared, all LDs and unready STs should be clear
   * if a LD is being done, interrupt it and remove the entry
   * if a ST is being done, do not interrupt or remove it
   *
   * details: clear all entrys in lsb_next, percolate lsb_now, push proper entrys into lsb_next
   */
  void Clear();

  /*
   * receive call from ls_rss(drop a LD/ST instruction)
   *       if ST: add to the queue, and put information on bus(so that rob can set ready)
   *       if LD: percolate lsb(from back to front)
   *              if there's a ST with same addr(overlap), put information on bus
   *              else add to queue
   */
  void Execute(OptType opt, int addr, int value, int label, CommonDataBus &cdb);

  /*
   * check count: if count > 0: a ld/st is undergoing, --count
   *              if count == 0: a ld/st is finished, (instruction at front is ready), (if LD)put on bus, (if ST)store in memory, pop
   *                             check if instruction at top is ready, if not, count = -1
   *                                                                   else, count = 3
   *              if count == -1: nothing is going on, still waiting
   *                              check the instruction at top, if it is ready, count = 3
   */
  void TryLoadStore(Memory &mem, CommonDataBus &cdb);

  // * for unready STs: set ready
  void CheckBus(const CommonDataBus &cdb);

private:
  CircularQueue<LsbEntry, LSBSIZE> lsb_now;
  CircularQueue<LsbEntry, LSBSIZE> lsb_next;
  int count = -1;
};

#endif //RISCV_SIMULATOR_LSB_H
