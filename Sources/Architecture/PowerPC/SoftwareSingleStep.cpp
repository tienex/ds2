//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PowerPC/SoftwareSingleStep.h"
#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Architecture::CPUState;
using ds2::Target::Process;

namespace ds2 {
namespace Architecture {
namespace PowerPC {

// PowerPC instruction opcodes
enum PowerPCOpcode {
  kOpcodeBC = 16,   // Branch Conditional (B-form)
  kOpcodeB = 18,    // Branch (I-form)
  kOpcodeXL = 19,   // XL-form (includes bclr, bcctr)
};

// XL-form extended opcodes
enum PowerPCXLExtended {
  kXOBCLR = 16,   // Branch Conditional to Link Register
  kXOBCCTR = 528, // Branch Conditional to Count Register
};

// BO field encoding (simplified)
// Bit 0: Don't decrement CTR
// Bit 1: Don't check condition
// Bit 2: Condition value to match (0=false, 1=true)
// Bit 3: Don't check CTR
// Bit 4: Branch hint (0=no hint)
#define BO_ALWAYS 0x14       // Branch always (1z1zz)
#define BO_CTR_COND_MASK 0x0C  // Mask for checking if branch uses condition

// Helper to sign-extend immediate values
static inline int64_t SignExtend(int64_t value, int bits) {
  int64_t m = 1LL << (bits - 1);
  return (value ^ m) - m;
}

// Check if branch is unconditional based on BO field
static inline bool IsUnconditionalBranch(uint32_t bo) {
  // Unconditional if bit 1 is set (don't check condition)
  // and bit 3 is set (don't check CTR)
  return (bo & 0x14) == 0x14;
}

// Check if branch condition is likely to be taken
// This is a simplified heuristic - real implementation would
// evaluate CR and CTR based on BO and BI fields
static inline bool EvaluateBranchCondition(const CPUState &state, uint32_t bo,
                                           uint32_t bi) {
  // BO field bits:
  // bit 0: 0=decrement CTR, 1=don't decrement
  // bit 1: 0=branch if cond false, 1=branch if cond true
  // bit 2: condition value to check
  // bit 3: 0=branch depends on CTR, 1=ignore CTR
  // bit 4: branch prediction hint

  // If unconditional, always taken
  if (IsUnconditionalBranch(bo)) {
    return true;
  }

  // Check CTR if required (bit 3 == 0)
  bool ctr_ok = true;
  if ((bo & 0x04) == 0) {
    // Decrement CTR if bit 0 == 0
    uint64_t ctr = state.ctr;
    if ((bo & 0x10) == 0) {
      ctr--;
    }
    // Check CTR condition (bit 1)
    if (bo & 0x02) {
      ctr_ok = (ctr == 0);
    } else {
      ctr_ok = (ctr != 0);
    }
  }

  // Check condition register if required (bit 2 == 0 in combined check)
  bool cond_ok = true;
  if ((bo & 0x10) == 0) {
    // Extract CR bit specified by BI
    uint32_t cr_bit = (state.cr >> (31 - bi)) & 1;
    // Check if CR bit matches expected value (bit 3 of BO)
    bool expected = (bo & 0x08) != 0;
    cond_ok = (cr_bit == expected);
  }

  return ctr_ok && cond_ok;
}

ErrorCode PrepareSoftwareSingleStep(Process *process,
                                    BreakpointManager *manager,
                                    CPUState const &state,
                                    Address const &address) {
  uint64_t pc = address.valid() ? address.value() : state.pc();
  uint32_t insn;

  // Read instruction at PC
  // PowerPC instructions are 4 bytes, may be big or little endian
  CHK(process->readMemory(pc, &insn, sizeof(insn)));

  // PowerPC is typically big-endian, but can run little-endian
  // We assume the debugger handles endianness conversion
  // If needed, byte swap here based on process endianness

  // Decode instruction
  uint32_t opcode = (insn >> 26) & 0x3F;  // Bits 0-5 (big-endian bit numbering)
  uint32_t bo = (insn >> 21) & 0x1F;      // Bits 6-10
  uint32_t bi = (insn >> 16) & 0x1F;      // Bits 11-15
  int32_t bd = (insn >> 2) & 0x3FFF;      // Bits 16-29 (14-bit signed offset)
  int32_t li = (insn >> 2) & 0xFFFFFF;    // Bits 6-29 (24-bit signed offset)
  uint32_t aa = (insn >> 1) & 0x1;        // Bit 30 (absolute address)
  uint32_t lk = insn & 0x1;               // Bit 31 (link)
  uint32_t xo = (insn >> 1) & 0x3FF;      // Extended opcode for XL-form

  bool isBranch = false;
  bool isConditional = false;
  uint64_t targetAddr = 0;
  uint64_t fallThroughAddr = pc + 4;

  switch (opcode) {
  case kOpcodeB: {
    // Unconditional branch (I-form)
    // b[l][a] target
    isBranch = true;
    isConditional = false;

    // Calculate target address
    int64_t offset = SignExtend(li, 24) << 2;  // Sign-extend 24 bits and shift left 2
    if (aa) {
      // Absolute address
      targetAddr = offset;
    } else {
      // Relative to PC
      targetAddr = pc + offset;
    }

    DS2LOG(Debug, "PowerPC B%s%s to %#llx at PC=%#llx",
           lk ? "L" : "", aa ? "A" : "", (unsigned long long)targetAddr,
           (unsigned long long)pc);
    break;
  }

  case kOpcodeBC: {
    // Conditional branch (B-form)
    // bc[l][a] BO, BI, target
    isBranch = true;
    isConditional = !IsUnconditionalBranch(bo);

    // Calculate target address
    int64_t offset = SignExtend(bd, 14) << 2;  // Sign-extend 14 bits and shift left 2
    if (aa) {
      // Absolute address
      targetAddr = offset;
    } else {
      // Relative to PC
      targetAddr = pc + offset;
    }

    bool likely = EvaluateBranchCondition(state, bo, bi);
    DS2LOG(Debug, "PowerPC BC%s%s BO=%#x, BI=%d to %#llx (%s) at PC=%#llx",
           lk ? "L" : "", aa ? "A" : "", bo, bi,
           (unsigned long long)targetAddr,
           likely ? "likely taken" : "likely not taken",
           (unsigned long long)pc);
    break;
  }

  case kOpcodeXL: {
    // XL-form instructions (bclr, bcctr)
    if (xo == kXOBCLR) {
      // Branch Conditional to Link Register
      // bclr[l] BO, BI
      isBranch = true;
      isConditional = !IsUnconditionalBranch(bo);
      targetAddr = state.lr & ~3ULL;  // Clear low 2 bits

      DS2LOG(Debug, "PowerPC BCLR%s BO=%#x, BI=%d to LR=%#llx at PC=%#llx",
             lk ? "L" : "", bo, bi, (unsigned long long)targetAddr,
             (unsigned long long)pc);
    } else if (xo == kXOBCCTR) {
      // Branch Conditional to Count Register
      // bcctr[l] BO, BI
      isBranch = true;
      isConditional = !IsUnconditionalBranch(bo);
      targetAddr = state.ctr & ~3ULL;  // Clear low 2 bits

      DS2LOG(Debug, "PowerPC BCCTR%s BO=%#x, BI=%d to CTR=%#llx at PC=%#llx",
             lk ? "L" : "", bo, bi, (unsigned long long)targetAddr,
             (unsigned long long)pc);
    }
    break;
  }
  }

  if (isBranch) {
    // For branches, set breakpoints at possible next locations

    // Always set breakpoint at branch target
    CHK(manager->add(targetAddr, BreakpointManager::Lifetime::TemporaryOneShot,
                     4, BreakpointManager::kModeExec));

    // For conditional branches, also set breakpoint at fall-through
    if (isConditional) {
      CHK(manager->add(fallThroughAddr,
                       BreakpointManager::Lifetime::TemporaryOneShot, 4,
                       BreakpointManager::kModeExec));
    }

    DS2LOG(Debug,
           "PowerPC single-step: PC=%#llx, target=%#llx, fallthrough=%#llx",
           (unsigned long long)pc, (unsigned long long)targetAddr,
           isConditional ? (unsigned long long)fallThroughAddr : 0ULL);
  } else {
    // Not a branch - just step to next instruction
    CHK(manager->add(fallThroughAddr,
                     BreakpointManager::Lifetime::TemporaryOneShot, 4,
                     BreakpointManager::kModeExec));

    DS2LOG(Debug, "PowerPC single-step: PC=%#llx, next=%#llx",
           (unsigned long long)pc, (unsigned long long)fallThroughAddr);
  }

  return kSuccess;
}

} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
