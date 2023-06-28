#include "cpu.h"

u8 CPU::run() {
  while (true) {
    TryIssue();
    ExecuteRss();
    AccessMem();
    std::pair<u8, bool> tmp = TryCommit();
    if (tmp.second) { // commit addi x0+255->x10(a0)时，输出并退出程序
      return tmp.first;
    }

    CheckBus();
    Flush();
    ++clk;
  }
}

void CPU::Flush() {
  rob.flush();
  reg.flush();
  lsb.flush();
  ls_rss.flush();
  ari_rss.flush();
  ready_bus.clear();
  commit_bus.clear();
}

/*
 * rob: check ready_bus(set ready and get value)
 * ls_rss, ari_rss: check ready_bus(clear dependency)
 * reg: check commit_bus(write data in x[rd], if dependency is same, clear dependency)
 * lsb: check commit_bus(for unready STs: set ready)
 */
void CPU::CheckBus() {
  rob.CheckBus(ready_bus);
  ls_rss.CheckBus(ready_bus);
  ari_rss.CheckBus(ready_bus);
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
}

/*
 * rob: check entry at front, if ready, commit, put on commit bus, remove entry
 *                            else return
 * handle .END, jalr and branch prediction
 *
 * Commit: .END: return {1, a0}
 *         prediction failed: return {2, correct_pc}
 *         else return {0, 0}
 */
std::pair<u8, bool> CPU::TryCommit() {
  std::pair<int, int> tmp = rob.Commit(commit_bus, reg);
  if (tmp.first == 1) return {tmp.second, true};
  if (tmp.first == 2) {
    pc = tmp.second;
    pc_start = true;
    ClearPipeline();
  }
  return {0, false};
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
  u32 next_code = 0;
  InstructionType next_type;
  if (pc_start) {
    next_code = mem.FetchWordReverse(pc);
    next_type = iu.GetInstructionType(next_code);
    pc_start = false;
  }
  else {
    int pc_checkpoint = pc;
    pc = iu.NextPc(predictor, pc);
    next_code = mem.FetchWordReverse(pc);
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
  InstructionUnit::Instruction next_ins = iu.DecodeSet(next_code, next_type);
  int index = rob.issue(next_ins, reg, pc);
  if (next_type == InstructionType::I || next_type == InstructionType::S) {
    ls_rss.issue(index, next_ins, reg);
  }
  else {
    ari_rss.issue(index, next_ins, reg);
  }
}