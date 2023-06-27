#include "cpu.h"

int CPU::run() {
  while (true) {
    TryIssue();
    ExecuteRss();
    AccessMem();
    commit()
    // commit addi a0+255->x10时，输出并退出程序
    flush(); // 里面要听一下bus，先从bus里拿信息(rss & rob)
    ++clk;
  }
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
 * decode next instruction: instruction_unit
 * if rob & rss is not full, issue an instruction in rob and rss
 * else, restore pc to checkpoint
 */
void CPU::TryIssue() {
  if (rob.full()) return;
  int pc_checkpoint = pc;
  pc = iu.NextPc(predictor);
  u32 next_code = memory.FetchWordReverse(pc);
  InstructionType next_type = iu.GetInstructionType(next_code);

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

  // issue
  InstructionUnit::Instruction next_ins = iu.DecodeSet(next_code, next_type);
  int index = rob.issue(next_ins, reg);
  if (next_type == InstructionType::I || next_type == InstructionType::S) {
    ls_rss.issue(index, next_ins, reg);
  }
  else {
    ari_rss.issue(index, next_ins, reg);
  }
}