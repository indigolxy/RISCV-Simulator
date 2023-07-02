#include "cpu.h"

u8 CPU::run() {
  bool debug_st = false;
  void (CPU::*func[4])() = {&CPU::TryIssue, &CPU::ExecuteRss, &CPU::AccessMem, &CPU::TryCommit};
  while (true) {
    std::shuffle(func, func + 4, std::mt19937(std::random_device()()));
//    if (clk == 1368) debug_st = true;
    if (debug_st) {
      std::cout << std::endl;
      std::cout << "clock cycle " << std::dec << clk << ": pc = " << std::hex << pc << std::dec << std::endl;
    }

    (this->*func[0])();
    if (debug_st) {
      std::cout << std::endl << "ROB_AFTER_ISSUE: " << std::endl;
      rob.Print();
      std::cout << std::endl << "LS_RSS_AFTER_ISSUE: " << std::endl;
      ls_rss.print();
      std::cout << "-----------------ARI_RSS_AFTER_ISSUE--------------------" << std::endl;
      ari_rss.print();
    }

    (this->*func[1])();
    if (debug_st) {
      std::cout << std::endl << "LS_RSS_AFTER_EXECUTE: " << std::endl;
      ls_rss.print();
      std::cout << std::endl << "LSB_AFTER_EXECUTE: " << std::endl;
      lsb.print();
      std::cout << "-----------------ARI_RSS_AFTER_EXECUTE--------------------" << std::endl;
      ari_rss.print();
    }

    (this->*func[2])();
    if (debug_st) {
      std::cout << std::endl << "LSB_AFTER_ACCESS_MEM: " << std::endl;
      lsb.print();
    }

    (this->*func[3])();
    if (debug_st) {
      std::cout << std::endl << "ROB_AFTER_COMMIT: " << std::endl;
      rob.Print();
//      std::cout << std::endl << "REGISTER: " << std::endl;
//      reg.print();
      std::cout << std::endl <<  "READY_BUS: " << std::endl;
      ready_bus.print();
      std::cout << std::endl << "COMMIT_BUS: " << std::endl;
      commit_bus.print();
    }

    CheckBus();
    Flush();

    if (end_flag) {
      return ret_value;
    }

//    std::cout << "-----------------ARI_RSS_END--------------------" << std::endl;
//    ari_rss.print();
//    if (clk == 1374) return -1;
    ++clk;
  }
}

void CPU::Flush() {
  if (jump_pc > 0) {
    ClearPipeline();
    pc = jump_pc;
    jump_pc = -1;
    pc_start = true;
    iu.stall = false;
  }
  rob.flush();
  reg.FlushSetX0();
  lsb.flush();
  ls_rss.flush();
  ari_rss.flush();
  ready_bus.clear();
  commit_bus.clear();
}

/*
 * rob: check ready_bus(set ready and get value)
 * ls_rss, ari_rss: check ready_bus and commit_bus (clear dependency)
 * reg: check commit_bus(write data in x[rd], if dependency is same, clear dependency)
 * lsb: check commit_bus(for unready STs: set ready)
 *
 * operate directly on the next state
 */
void CPU::CheckBus() {
  rob.CheckBus(ready_bus);
  ls_rss.CheckBus(ready_bus, commit_bus);
  ari_rss.CheckBus(ready_bus, commit_bus);
  reg.CheckBus(commit_bus);
  lsb.CheckBus(commit_bus);
}

/*
 * called when prediction failed
 * clear all entries in rob, ari_rss, ls_rss, lsb
 * clear all dependency in reg
 */
void CPU::ClearPipeline() {
  rob.Clear();
  ari_rss.Clear();
  ls_rss.Clear();
  lsb.Clear();
  reg.ClearDependency();
  ready_bus.clear();
  commit_bus.clear();
}

/*
 * rob: check entry at front, if ready, commit, put on commit bus, remove entry
 *                            else return
 * handle .END, jalr and branch prediction
 *
 * Commit: .END: set end_flag and ret_value
 *         prediction failed: set jump_pc
 */
void CPU::TryCommit() {
//  if (clk % 100000 == 0) std::cout << "clk = " << clk << std::endl;
  std::pair<int, int> tmp = rob.Commit(commit_bus, reg, predictor);
  if (tmp.first == 1) {
    end_flag = true;
    ret_value = tmp.second;
  }
  if (tmp.first == 2) {
    // 下个周期才更新pc，这个周期最后flush的时候才clearpipeline
    jump_pc = tmp.second;
  }
}

/*
 * lsb check and try access memory(load or store)
 * if a ld or store is finished, put information on bus and pop
 */
void CPU::AccessMem() {
  lsb.TryLoadStore(mem, ready_bus);
}

/*
 * execute in ari_rss: find an entry without dependency and calculate in ALU and get result
 *                    put the information into bus(label, value), remove entry
 * execute in ls_rss: find an entry without dependency
 *                    if a ST is at top and without dependency,
 *                         calculate its addr and value, pop it into lsb and remove entry
 *                    if a LD is without dependency and has no STs before it,
 *                         calculate its addr, pop it into lsb(and then lsb.execute) and remove entry
 * execute in lsb: receive call from ls_rss(drop a LD/ST instruction)
 *                 if ST: add to the queue
 *                 if LD: percolate lsb, if there's a ST with same addr, put information on bus
 *                                       else add to queue
 */
void CPU::ExecuteRss() {
  ari_rss.AriExecute(alu, ready_bus, pc);
  ls_rss.LsExecute(alu, ready_bus, lsb);
}

/*
 * get next instruction: get next pc(+4 or jump or predict)
 *                       if pc_start, read current pc(used cases: the very beginning or after clean pipeline)
 * decode next instruction: instruction_unit
 * if rob & rss is not full, issue an instruction in rob and rss
 * else, restore pc to checkpoint
 */
void CPU::TryIssue() {
  if (rob.full()) return;
//  if (jump_pc > 0) {
//    iu.stall = false;
//  }
  if (iu.stall) return;
  u32 next_code = 0;
  InstructionType next_type;
  int pc_checkpoint = pc;
  if (pc_start) {
    next_code = mem.LoadWord(pc);
    next_type = iu.GetInstructionType(next_code);
    pc_start = false;
  }
  else {
    pc = iu.NextPc(predictor, pc);
    if (pc == -1) {
      pc = pc_checkpoint;
      return;
    }
    next_code = mem.LoadWord(pc);
    next_type = iu.GetInstructionType(next_code);

    if (next_type == InstructionType::I || next_type == InstructionType::S) {
      if (ls_rss.full()) {
        pc = pc_checkpoint;
        return;
      }
    }
    else {
      if (ari_rss.full()) {
        pc = pc_checkpoint;
        return;
      }
    }
  }

  // issue
  if (next_code == 0x0ff00513) iu.stall = true;
  InstructionUnit::Instruction next_ins = iu.DecodeSet(next_code, next_type);
  int index = rob.issue(next_ins, reg, pc);
  if ((next_ins.opt == OptType::LB || next_ins.opt == OptType::LH || next_ins.opt == OptType::LW || next_ins.opt == OptType::LBU || next_ins.opt == OptType::LHU) || next_type == InstructionType::S) {
    ls_rss.issue(index, next_ins, reg, pc);
  }
  else {
    ari_rss.issue(index, next_ins, reg, pc);
  }
}