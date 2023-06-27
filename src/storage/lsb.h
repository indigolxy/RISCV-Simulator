
#ifndef RISCV_SIMULATOR_LSB_H
#define RISCV_SIMULATOR_LSB_H

#include "../utils/circular_queue.h"
#include "../utils/config.h"
#include "../units/instuction.h"
#include "../storage/memory.h"

class LoadStoreBuffer {
private:
  struct LsbEntry {
    OptType opt;
    int addr = 0;
    int value = 0;
    int label = 0;
  };

public:
  LoadStoreBuffer() = default;

  /*
   * receive call from ls_rss(drop a LD/ST instruction)
   *       if ST: add to the queue
   *       if LD: percolate lsb(from back to front)
   *              if there's a ST with same addr(overlap), put information on bus
   *              else add to queue
   */
  void Execute(OptType opt, int addr, int value, int label, CommonDataBus &cdb) {
    if (opt == OptType::SB || opt == OptType::SH || opt == OptType::SW) {
      lsb_next.push({opt, addr, value, label});
      return;
    }

    // percolate lsb, find ST with overlapped units
    CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.back();
    while (true) {
      if (iter->opt == OptType::SB) {
        int tmp = Memory::GetByte(iter->value);
        if (opt == OptType::LB && iter->addr == addr) {
          cdb.PutOnBus(label, Memory::SignExtend(tmp, 8));
          return;
        }
        if (opt == OptType::LBU && iter->addr == addr) {
          cdb.PutOnBus(label, tmp);
          return;
        }
      }

      if (iter->opt == OptType::SH) {
        int tmp = Memory::GetHalf(iter->value);
        if (opt == OptType::LB) {
          if (addr == iter->addr) {
            cdb.PutOnBus(label, Memory::SignExtend(Memory::GetHighByte(tmp), 8));
            return;
          }
          if (addr == iter->addr + 1) {
            cdb.PutOnBus(label, Memory::SignExtend(Memory::GetByte(tmp), 8));
            return;
          }
        }
        if (opt == OptType::LBU) {
          if (addr == iter->addr) {
            cdb.PutOnBus(label, Memory::GetHighByte(tmp));
            return;
          }
          if (addr == iter->addr + 1) {
            cdb.PutOnBus(label, Memory::GetByte(tmp));
            return;
          }
        }
        if (opt == OptType::LH && addr == iter->addr) {
          cdb.PutOnBus(label, Memory::SignExtend(tmp, 16));
          return;
        }
        if (opt == OptType::LHU && addr == iter->addr) {
          cdb.PutOnBus(label, tmp);
          return;
        }
      }

      if (iter->opt == OptType::SW) {
        int tmp = iter->value;
        // todo
      }

      if (iter == lsb_now.front()) break;
      --iter;
    }

    // todo push to queue
  }


private:
  CircularQueue<LsbEntry, LSBSIZE> lsb_now;
  CircularQueue<LsbEntry, LSBSIZE> lsb_next;
  int count;
};

#endif //RISCV_SIMULATOR_LSB_H
