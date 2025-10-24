//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/MIPS64/SoftwareSingleStep.h"
#include "DebugServer2/Architecture/MIPS64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Architecture::CPUState;
using ds2::Target::Process;

namespace ds2 {
namespace Architecture {
namespace MIPS64 {

// MIPS instruction opcodes and function codes
enum MIPSOpcode {
  kOpcodeSpecial = 0x00,  // Special (includes JR, JALR)
  kOpcodeRegimm = 0x01,   // REGIMM (includes BLTZ, BGEZ, etc.)
  kOpcodeJ = 0x02,        // J (jump)
  kOpcodeJAL = 0x03,      // JAL (jump and link)
  kOpcodeBEQ = 0x04,      // BEQ (branch equal)
  kOpcodeBNE = 0x05,      // BNE (branch not equal)
  kOpcodeBLEZ = 0x06,     // BLEZ (branch less than or equal zero)
  kOpcodeBGTZ = 0x07,     // BGTZ (branch greater than zero)
  kOpcodeBeql = 0x14,     // BEQL (branch equal likely)
  kOpcodeBnel = 0x15,     // BNEL (branch not equal likely)
  kOpcodeBlezl = 0x16,    // BLEZL (branch less than or equal zero likely)
  kOpcodeBgtzl = 0x17,    // BGTZL (branch greater than zero likely)
  kOpcodeCop1 = 0x11,     // Coprocessor 1 (FPU branches)
  kOpcodeCop2 = 0x12,     // Coprocessor 2
  kOpcodeDaddi = 0x18,    // DADDI (64-bit add immediate)
};

enum MIPSSpecialFunc {
  kFuncJR = 0x08,   // JR (jump register)
  kFuncJALR = 0x09, // JALR (jump and link register)
};

enum MIPSRegimmRt {
  kRegimmBLTZ = 0x00,   // BLTZ (branch less than zero)
  kRegimmBGEZ = 0x01,   // BGEZ (branch greater than or equal zero)
  kRegimmBltzl = 0x02,  // BLTZL (branch less than zero likely)
  kRegimmBgezl = 0x03,  // BGEZL (branch greater than or equal zero likely)
  kRegimmBLTZAL = 0x10, // BLTZAL (branch less than zero and link)
  kRegimmBGEZAL = 0x11, // BGEZAL (branch greater than or equal zero and link)
};

// Helper to sign-extend immediate values
static inline int64_t SignExtend(int64_t value, int bits) {
  int64_t m = 1LL << (bits - 1);
  return (value ^ m) - m;
}

// Get register value with 0 for register 0
static inline uint64_t GetRegValue(const CPUState &state, int reg) {
  if (reg == 0)
    return 0;
  return state.gp.regs[reg];
}

ErrorCode PrepareMIPS64SoftwareSingleStep(
    Process *process, uint64_t pc, CPUState const &state, uint64_t &nextPC,
    uint32_t &nextPCSize, uint64_t &branchPC, uint32_t &branchPCSize) {
  uint32_t insn;

  // Read instruction at PC
  CHK(process->readMemory(pc, &insn, sizeof(insn)));

  // Decode instruction
  uint32_t opcode = (insn >> 26) & 0x3F;
  uint32_t rs = (insn >> 21) & 0x1F;
  uint32_t rt = (insn >> 16) & 0x1F;
  uint32_t rd = (insn >> 11) & 0x1F;
  uint32_t func = insn & 0x3F;
  int32_t imm = static_cast<int16_t>(insn & 0xFFFF);
  uint32_t target = insn & 0x3FFFFFF;

  bool isBranch = false;
  bool takeBranch = false;
  uint64_t targetAddr = 0;

  // MIPS has branch delay slots - instruction after branch always executes
  // So nextPC is always PC + 8 (after delay slot) for branches
  // and branchPC is the branch target

  switch (opcode) {
  case kOpcodeSpecial:
    // Check for JR and JALR
    if (func == kFuncJR || func == kFuncJALR) {
      isBranch = true;
      takeBranch = true;
      targetAddr = GetRegValue(state, rs);
      DS2LOG(Debug, "MIPS64 %s to reg r%d = %#llx at PC=%#llx",
             func == kFuncJR ? "JR" : "JALR", rs,
             (unsigned long long)targetAddr, (unsigned long long)pc);
    }
    break;

  case kOpcodeJ:
  case kOpcodeJAL:
    // Unconditional jump
    isBranch = true;
    takeBranch = true;
    // For MIPS64, target is (PC+4)[63:28] || target || 00
    targetAddr = ((pc + 4) & 0xFFFFFFFFF0000000ULL) | (target << 2);
    DS2LOG(Debug, "MIPS64 %s to %#llx at PC=%#llx",
           opcode == kOpcodeJ ? "J" : "JAL", (unsigned long long)targetAddr,
           (unsigned long long)pc);
    break;

  case kOpcodeBEQ:
  case kOpcodeBeql:
    // Branch if equal
    isBranch = true;
    takeBranch = (GetRegValue(state, rs) == GetRegValue(state, rt));
    targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);
    DS2LOG(Debug, "MIPS64 BEQ r%d(=%#llx), r%d(=%#llx) -> %s at PC=%#llx", rs,
           (unsigned long long)GetRegValue(state, rs), rt,
           (unsigned long long)GetRegValue(state, rt),
           takeBranch ? "taken" : "not taken", (unsigned long long)pc);
    break;

  case kOpcodeBNE:
  case kOpcodeBnel:
    // Branch if not equal
    isBranch = true;
    takeBranch = (GetRegValue(state, rs) != GetRegValue(state, rt));
    targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);
    DS2LOG(Debug, "MIPS64 BNE r%d, r%d -> %s at PC=%#llx", rs, rt,
           takeBranch ? "taken" : "not taken", (unsigned long long)pc);
    break;

  case kOpcodeBLEZ:
  case kOpcodeBlezl:
    // Branch if less than or equal to zero
    isBranch = true;
    takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) <= 0);
    targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);
    DS2LOG(Debug, "MIPS64 BLEZ r%d -> %s at PC=%#llx", rs,
           takeBranch ? "taken" : "not taken", (unsigned long long)pc);
    break;

  case kOpcodeBGTZ:
  case kOpcodeBgtzl:
    // Branch if greater than zero
    isBranch = true;
    takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) > 0);
    targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);
    DS2LOG(Debug, "MIPS64 BGTZ r%d -> %s at PC=%#llx", rs,
           takeBranch ? "taken" : "not taken", (unsigned long long)pc);
    break;

  case kOpcodeRegimm:
    // REGIMM instructions (BLTZ, BGEZ, etc.)
    isBranch = true;
    targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);

    switch (rt) {
    case kRegimmBLTZ:
    case kRegimmBltzl:
      takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) < 0);
      DS2LOG(Debug, "MIPS64 BLTZ r%d -> %s at PC=%#llx", rs,
             takeBranch ? "taken" : "not taken", (unsigned long long)pc);
      break;
    case kRegimmBGEZ:
    case kRegimmBgezl:
      takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) >= 0);
      DS2LOG(Debug, "MIPS64 BGEZ r%d -> %s at PC=%#llx", rs,
             takeBranch ? "taken" : "not taken", (unsigned long long)pc);
      break;
    case kRegimmBLTZAL:
      takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) < 0);
      DS2LOG(Debug, "MIPS64 BLTZAL r%d -> %s at PC=%#llx", rs,
             takeBranch ? "taken" : "not taken", (unsigned long long)pc);
      break;
    case kRegimmBGEZAL:
      takeBranch = (static_cast<int64_t>(GetRegValue(state, rs)) >= 0);
      DS2LOG(Debug, "MIPS64 BGEZAL r%d -> %s at PC=%#llx", rs,
             takeBranch ? "taken" : "not taken", (unsigned long long)pc);
      break;
    default:
      isBranch = false;
      break;
    }
    break;

  case kOpcodeCop1: {
    // FPU branch instructions (BC1F, BC1T, etc.)
    uint32_t fmt = rs;
    if (fmt == 0x08) { // BC1
      isBranch = true;
      uint32_t cc = (rt >> 2) & 0x7;
      bool tf = (rt & 0x01) != 0;
      // Check FPU condition code - this is simplified
      // Real implementation would need to check state.fpu.fcsr
      takeBranch = tf; // Simplified
      targetAddr = pc + 4 + (SignExtend(imm, 16) << 2);
      DS2LOG(Debug, "MIPS64 BC1 cc=%d, tf=%d -> %s at PC=%#llx", cc, tf,
             takeBranch ? "taken" : "not taken", (unsigned long long)pc);
    }
    break;
  }
  }

  if (isBranch) {
    // For branches/jumps, we need to set breakpoints at:
    // 1. The delay slot instruction (PC + 4)
    // 2. The instruction after delay slot (PC + 8) if conditional
    // 3. The branch target

    // For unconditional branches (J, JAL, JR, JALR), only delay slot matters
    if (takeBranch) {
      // Set breakpoint at target
      branchPC = targetAddr;
      branchPCSize = 4;
    }

    // Always set breakpoint after delay slot for conditional branches
    if (opcode == kOpcodeBEQ || opcode == kOpcodeBNE || opcode == kOpcodeBLEZ ||
        opcode == kOpcodeBGTZ || opcode == kOpcodeRegimm ||
        opcode == kOpcodeBeql || opcode == kOpcodeBnel ||
        opcode == kOpcodeBlezl || opcode == kOpcodeBgtzl ||
        opcode == kOpcodeCop1) {
      // Conditional branch - set breakpoint after delay slot
      nextPC = pc + 8;
      nextPCSize = 4;
    } else {
      // Unconditional jump - only target matters, but we still need to
      // execute the delay slot, so no nextPC breakpoint needed
      nextPC = static_cast<uint64_t>(-1);
    }
  } else {
    // Not a branch - just step to next instruction
    nextPC = pc + 4;
    nextPCSize = 4;
  }

  return kSuccess;
}

ErrorCode PrepareMIPS16SoftwareSingleStep(Process *process, uint64_t pc,
                                          CPUState const &state,
                                          uint64_t &nextPC,
                                          uint32_t &nextPCSize,
                                          uint64_t &branchPC,
                                          uint32_t &branchPCSize) {
  // MIPS16 is a compressed instruction set
  // For now, return unsupported - would need full MIPS16 decoder
  // MIPS16 has 16-bit and 32-bit (extended) instructions
  DS2LOG(Warning, "MIPS16 software single-step not yet implemented");
  return kErrorUnsupported;
}

ErrorCode PrepareMicroMIPSSoftwareSingleStep(
    Process *process, uint64_t pc, CPUState const &state, uint64_t &nextPC,
    uint32_t &nextPCSize, uint64_t &branchPC, uint32_t &branchPCSize) {
  // microMIPS is another compressed instruction set
  // For now, return unsupported - would need full microMIPS decoder
  // microMIPS has 16-bit and 32-bit instructions with different encoding
  DS2LOG(Warning, "microMIPS software single-step not yet implemented");
  return kErrorUnsupported;
}

ErrorCode PrepareSoftwareSingleStep(Process *process,
                                    BreakpointManager *manager,
                                    CPUState const &state,
                                    Address const &address) {
  uint64_t pc = address.valid() ? address.value() : state.pc();
  uint64_t nextPC = static_cast<uint64_t>(-1);
  uint32_t nextPCSize = 0;
  uint64_t branchPC = static_cast<uint64_t>(-1);
  uint32_t branchPCSize = 0;

  // Check instruction mode from PC LSB
  // Bit 0 set means MIPS16 or microMIPS mode
  if (pc & 0x1) {
    // Need to determine if it's MIPS16 or microMIPS
    // For now, we'll try MIPS16 first
    // In a real implementation, this would check CPU mode bits
    pc &= ~1ULL; // Clear mode bit
    DS2LOG(Debug, "Attempting MIPS16 single-step at PC=%#llx",
           (unsigned long long)pc);
    CHK(PrepareMIPS16SoftwareSingleStep(process, pc, state, nextPC, nextPCSize,
                                        branchPC, branchPCSize));
  } else {
    // Normal MIPS64 mode
    CHK(PrepareMIPS64SoftwareSingleStep(process, pc, state, nextPC, nextPCSize,
                                        branchPC, branchPCSize));
  }

  DS2LOG(Debug, "PC=%#llx, branchPC=%#llx[size=%d] nextPC=%#llx[size=%d]",
         (unsigned long long)pc, (unsigned long long)branchPC, branchPCSize,
         (unsigned long long)nextPC, nextPCSize);

  // Set breakpoints
  if (branchPC != static_cast<uint64_t>(-1)) {
    DS2ASSERT(branchPCSize != 0);
    CHK(manager->add(branchPC, BreakpointManager::Lifetime::TemporaryOneShot,
                     branchPCSize, BreakpointManager::kModeExec));
  }

  if (nextPC != static_cast<uint64_t>(-1)) {
    DS2ASSERT(nextPCSize != 0);
    CHK(manager->add(nextPC, BreakpointManager::Lifetime::TemporaryOneShot,
                     nextPCSize, BreakpointManager::kModeExec));
  }

  return kSuccess;
}
} // namespace MIPS64
} // namespace Architecture
} // namespace ds2
