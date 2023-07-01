#include "lsb.h"

void LoadStoreBuffer::print() {
  std::cout << "count = " << count << std::endl;
  std::cout << "----------------LSB_NOW--------------------" << std::endl;
  lsb_now.print();
  std::cout << "----------------LSB_NEXT--------------------" << std::endl;
  lsb_next.print();
}

void LoadStoreBuffer::flush() {
  lsb_now = lsb_next;
}

void LoadStoreBuffer::CheckBus(const CommonDataBus &cdb) {
  if (lsb_next.empty()) return;
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

void LoadStoreBuffer::Execute(OptType opt, int addr, int value, int label, CommonDataBus &cdb)  {
  if (opt == OptType::SB || opt == OptType::SH || opt == OptType::SW) {
    int tmp = lsb_next.push({-1, false, opt, addr, value, label}); // ST: not ready
    lsb_next.back()->cnt = tmp;
    cdb.PutOnBus(label, value);
    return;
  }

  // percolate lsb, find ST with overlapped units
  if (!lsb_now.empty()) {
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
  }

  int tmp = lsb_next.push({-1, true, opt, addr, value, label}); // LD: ready
  lsb_next.back()->cnt = tmp;
}

void LoadStoreBuffer::TryLoadStore(Memory &mem, CommonDataBus &cdb) {
  if (count > 0) {
    --count;
    return;
  }
  if (lsb_now.empty()) {
    count = -1;
    return; // lsb is empty, count = -1, waiting
  }
  CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.front();
  if (count == 0) {
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

  if (iter == lsb_now.end() || !iter->ready) {
    count = -1;
  }
  else {
    count = 3;
  }
}

void LoadStoreBuffer::Clear() {
  lsb_next.clear();
  if (lsb_now.empty()) {
    count = -1;
    return;
  }
  CircularQueue<LsbEntry, LSBSIZE>::iterator iter = lsb_now.front();

  // deal with the undergoing process
  bool interrupted = false;
  if (count >= 0) {
    // LD is being done, do not push the entry into lsb_next, interrupt it
    if (iter->opt != OptType::SB && iter->opt != OptType::SH && iter->opt != OptType::SW) {
      interrupted = true;
    }
    else { // ST is being done, push into lsb_next, do not interrupt
      int tmp = lsb_next.push(*iter);
      lsb_next.back()->cnt = tmp;
    }
  }

  // other entrys
  ++iter;
  while (iter != lsb_now.end()) {
    if (iter->opt == OptType::SB || iter->opt == OptType::SH || iter->opt == OptType::SW) {
      if (iter->ready) {
        int tmp = lsb_next.push(*iter);
        lsb_next.back()->cnt = tmp;
      }
    }
    ++iter;
  }

  // set new counter: if the top is a ready ST, start the ST(count = 3)
  //                  else, lsb_next is empty, count = -1;
  if (interrupted) {
    (lsb_next.empty()) ? count = -1 : count = 3;
  }
}