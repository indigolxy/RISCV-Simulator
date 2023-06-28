
#ifndef RISCV_SIMULATOR_LSB_H
#define RISCV_SIMULATOR_LSB_H

#include "../utils/circular_queue.h"
#include "../utils/config.h"
#include "../units/instuction.h"
#include "../storage/memory.h"

class LoadStoreBuffer {
private:
  struct LsbEntry {
    bool ready;
    OptType opt;
    int addr = 0;
    int value = 0;
    int label = 0;
  };

public:
  LoadStoreBuffer() = default;

  void flush() {
    lsb_now = lsb_next;
  }

  /*
   * all ready ST shouldn't be cleared, all LDs and unready STs should be clear
   * if a LD is being done, interrupt it and remove the entry
   * if a ST is being done, do not interrupt or remove it
   *
   * details: clear all entrys in lsb_next, percolate lsb_now, push proper entrys into lsb_next
   */
  void Clear() {
    lsb_next.clear();
    CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.front();

    // deal with the undergoing process
    bool interrupted = false;
    if (count > 0) {
      // LD is being done, do not push the entry into lsb_next, interrupt it
      if (iter->opt != OptType::SB && iter->opt != OptType::SH && iter->opt != OptType::SW) {
        interrupted = true;
      }
      else { // ST is being done, push into lsb_next, do not interrupt
        lsb_next.push(*iter);
      }
    }

    // other entrys
    ++iter;
    while (iter != lsb_now.end()) {
      if (iter->opt == OptType::SB || iter->opt == OptType::SH || iter->opt == OptType::SW) {
        if (iter->ready) {
          lsb_next.push(*iter);
        }
      }
      ++iter;
    }

    // set new counter: if the top is a ready ST, start the ST(count = 3)
    //                  else, lsb_next is empty, count = 0;
    if (interrupted) {
      (lsb_next.empty()) ? count = 0 : count = 3;
    }
  }

  /*
   * receive call from ls_rss(drop a LD/ST instruction)
   *       if ST: add to the queue
   *       if LD: percolate lsb(from back to front)
   *              if there's a ST with same addr(overlap), put information on bus
   *              else add to queue
   */
  void Execute(OptType opt, int addr, int value, int label, CommonDataBus &cdb) {
    if (opt == OptType::SB || opt == OptType::SH || opt == OptType::SW) {
      lsb_next.push({false, opt, addr, value, label}); // ST: not ready
      return;
    }

    // percolate lsb, find ST with overlapped units
    CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.back();
    while (true) {
      if (iter->opt == OptType::SB) {
        int tmp = Memory::GetByte(iter->value);
        if ((opt == OptType::LB || opt == OptType::LBU) && iter->addr == addr) {
          cdb.PutOnBus(label, (opt == OptType::LB) ? Memory::SignExtend(tmp, 8) : tmp);
          return;
        }
      }

      if (iter->opt == OptType::SH) {
        int tmp = Memory::GetHalf(iter->value);
        if (opt == OptType::LB || opt == OptType::LBU) {
          if (addr == iter->addr) {
            tmp = Memory::GetHighByte(tmp);
            cdb.PutOnBus(label, (opt == OptType::LB) ? Memory::SignExtend(tmp, 8) : tmp);
            return;
          }
          if (addr == iter->addr + 1) {
            tmp = Memory::GetByte(tmp);
            cdb.PutOnBus(label, (opt == OptType::LBU) ? Memory::SignExtend(tmp, 8) : tmp);
            return;
          }
        }
        if ((opt == OptType::LH || opt == OptType::LHU) && addr == iter->addr) {
          cdb.PutOnBus(label, (opt == OptType::LH) ? Memory::SignExtend(tmp, 16) : tmp);
          return;
        }
      }

      if (iter->opt == OptType::SW) {
        int tmp = iter->value;
        if ((opt == OptType::LB || opt == OptType::LBU) && (addr >= iter->addr && addr <= iter->addr + 3)) {
          if (addr == iter->addr) tmp = Memory::GetHighByte(Memory::GetHighHalf(tmp));
          else if (addr == iter->addr + 1) tmp = Memory::GetByte(Memory::GetHighHalf(tmp));
          else if (addr == iter->addr + 2) tmp = tmp = Memory::GetHighByte(Memory::GetHalf(tmp));
          else tmp = Memory::GetByte(tmp);
          cdb.PutOnBus(label, (opt == OptType::LB) ? Memory::SignExtend(tmp, 8) : tmp);
          return;
        }
        if ((opt == OptType::LH || opt == OptType::LHU) && (addr >= iter->addr && addr <= iter->addr + 2)) {
          if (addr == iter->addr) tmp = Memory::GetHighHalf(tmp);
          else if (addr == iter->addr + 1) tmp = Memory::GetMidHalf(tmp);
          else tmp = Memory::GetHalf(tmp);
          cdb.PutOnBus(label, (opt == OptType::LH) ? Memory::SignExtend(tmp, 16) : tmp);
          return;
        }
        if (opt == OptType::LW && addr == iter->addr) {
          cdb.PutOnBus(label, tmp);
          return;
        }
      }

      if (iter == lsb_now.front()) break;
      --iter;
    }

    lsb_next.push({true, opt, addr, value, label}); // LD: ready
  }

  /*
   * check count: if count > 0 --count
   *              if count == 0: if instruction at front is ready, it's finished, (if LD)put on bus, pop
   *                             check if instruction at top is ready, if not, wait
   *                                                                   else, count = 3
   */
  void TryLoadStore(Memory &mem, CommonDataBus &cdb) {
    if (count > 0) {
      --count;
      return;
    }

    if (lsb_now.empty()) return;
    CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.front();
    if (iter->ready) {
      if (iter->opt == OptType::SB) {
        mem.StoreByte(iter->addr, iter->value);
      }
      else if (iter->opt == OptType::SH) {
        mem.StoreHalf(iter->addr, iter->value);
      }
      else if (iter->opt == OptType::SW) {
        mem.StoreWord(iter->addr, iter->value);
      }

      else if (iter->opt == OptType::LB) {
        cdb.PutOnBus(iter->label, Memory::SignExtend(mem.LoadByte(iter->addr), 8));
      }
      else if (iter->opt == OptType::LBU) {
        cdb.PutOnBus(iter->label, int(mem.LoadByte(iter->addr)));
      }
      else if (iter->opt == OptType::LH) {
        cdb.PutOnBus(iter->label, Memory::SignExtend(mem.LoadHalf(iter->addr), 16));
      }
      else if (iter->opt == OptType::LHU) {
        cdb.PutOnBus(iter->label, int(mem.LoadHalf(iter->addr)));
      }
      else if (iter->opt == OptType::LW) {
        cdb.PutOnBus(iter->label, int(mem.LoadWord(iter->addr)));
      }
      else throw std::exception();
      lsb_next.pop();
      ++iter;
    }

    if (iter->ready) {
      count = 3;
    }
  }

  // * for unready STs: set ready
  void CheckBus(const CommonDataBus &cdb) {
    CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_next.front();
    while (iter != lsb_next.end()) {
      if (iter->opt == OptType::SB || iter->opt == OptType::SH || iter->opt == OptType::SW) {
        if (!iter->ready) {
          if (cdb.TryGetValue(iter->label).first) iter->ready = true;
        }
      }
      ++iter;
    }
  }

private:
  CircularQueue<LsbEntry, LSBSIZE> lsb_now;
  CircularQueue<LsbEntry, LSBSIZE> lsb_next;
  int count = 0;
};

#endif //RISCV_SIMULATOR_LSB_H
