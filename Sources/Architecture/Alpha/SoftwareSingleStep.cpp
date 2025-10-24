//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Alpha Software Single-Step Implementation
//

#include "DebugServer2/Architecture/Alpha/SoftwareSingleStep.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace Alpha {

//
// Alpha Instruction Decoding for Single-Step
//
// Alpha uses fixed-length 32-bit instructions with the following formats:
//
// Memory format:    [opcode:6][Ra:5][Rb:5][displacement:16]
// Branch format:    [opcode:6][Ra:5][displacement:21]
// Operate format:   [opcode:6][Ra:5][Rb:5][SBZ:3][function:7][Rc:5]
// Floating format:  [opcode:6][Fa:5][Fb:5][function:11][Fc:5]
// PAL format:       [opcode:6][function:26]
//
// Key branch instructions:
//   BR, BSR  - Unconditional branch/branch to subroutine
//   BEQ, BNE, BLT, BLE, BGT, BGE - Integer conditional branches
//   BLBC, BLBS - Branch on low bit clear/set
//   FBEQ, FBNE, FBLT, FBLE, FBGT, FBGE - FP conditional branches
//   JMP, JSR, RET, JSR_COROUTINE - Jump instructions
//

// Extract opcode (bits 26-31)
static inline uint32_t GetOpcode(uint32_t insn) {
  return (insn >> 26) & 0x3F;
}

// Extract Ra register (bits 21-25)
static inline uint32_t GetRa(uint32_t insn) {
  return (insn >> 21) & 0x1F;
}

// Extract Rb register (bits 16-20)
static inline uint32_t GetRb(uint32_t insn) {
  return (insn >> 16) & 0x1F;
}

// Extract function code (bits 5-11 for operate format)
static inline uint32_t GetFunction(uint32_t insn) {
  return (insn >> 5) & 0x7F;
}

// Extract branch displacement (21 bits, sign-extended)
static inline int64_t GetBranchDisplacement(uint32_t insn) {
  int32_t disp = (insn & 0x1FFFFF);
  // Sign extend from 21 bits
  if (disp & 0x100000)
    disp |= 0xFFE00000;
  return (int64_t)disp * 4; // Multiply by 4 (instruction size)
}

// Extract memory displacement (16 bits, sign-extended)
static inline int64_t GetMemoryDisplacement(uint32_t insn) {
  int16_t disp = (int16_t)(insn & 0xFFFF);
  return (int64_t)disp;
}

// Check if instruction is a branch
static bool IsBranch(uint32_t insn, uint32_t opcode) {
  // Conditional branches (opcodes 0x30-0x3F)
  if (opcode >= 0x30 && opcode <= 0x3F)
    return true;

  return false;
}

// Check if instruction is an unconditional branch
static bool IsUnconditionalBranch(uint32_t insn, uint32_t opcode) {
  // BR (0x30), BSR (0x34)
  return (opcode == 0x30 || opcode == 0x34);
}

// Check if instruction is a jump
static bool IsJump(uint32_t insn, uint32_t opcode) {
  // JMP/JSR/RET/JSR_COROUTINE (opcode 0x1A)
  if (opcode == 0x1A) {
    uint32_t function = GetFunction(insn);
    // JMP=0, JSR=1, RET=2, JSR_COROUTINE=3
    return (function <= 3);
  }
  return false;
}

int PrepareSoftwareSingleStep(Target::ProcessBase *process,
                              BreakpointManager *manager,
                              CPUState const &state, Address const &address) {
  Address pc = address.valid() ? address : state.pc();

  // Read the instruction at PC
  uint32_t insn;
  if (process->readMemory(pc, &insn, sizeof(insn), nullptr) != kSuccess) {
    DS2LOG(Error, "cannot read instruction at %#" PRIx64, (uint64_t)pc);
    return -1;
  }

  uint32_t opcode = GetOpcode(insn);
  Address nextPC = pc + 4; // Default next instruction
  Address branchTarget;
  bool conditional = false;

  if (IsBranch(insn, opcode)) {
    // Branch instruction
    int64_t displacement = GetBranchDisplacement(insn);
    branchTarget = pc + 4 + displacement;
    conditional = !IsUnconditionalBranch(insn, opcode);

    DS2LOG(Debug, "branch at %#" PRIx64 " to %#" PRIx64 " (conditional=%d)",
           (uint64_t)pc, (uint64_t)branchTarget, conditional);
  } else if (IsJump(insn, opcode)) {
    // Jump instruction (JMP, JSR, RET, JSR_COROUTINE)
    uint32_t rb = GetRb(insn);

    if (rb < 32) {
      // Target is (Rb & ~3)
      branchTarget = state.alpha.gp.regs[rb] & ~3ULL;
      conditional = false; // Jumps are unconditional

      DS2LOG(Debug, "jump at %#" PRIx64 " to %#" PRIx64 " (via r%u)",
             (uint64_t)pc, (uint64_t)branchTarget, rb);
    } else {
      DS2LOG(Warning, "invalid jump instruction at %#" PRIx64, (uint64_t)pc);
      return -1;
    }
  }

  // Set breakpoints
  int count = 0;

  if (branchTarget.valid()) {
    // Set breakpoint at branch/jump target
    manager->add(branchTarget, BreakpointManager::kTypeSoftwareSingleStep);
    count++;

    if (conditional) {
      // Also set breakpoint at fall-through instruction
      manager->add(nextPC, BreakpointManager::kTypeSoftwareSingleStep);
      count++;
    }
  } else {
    // Not a branch/jump, just set breakpoint at next instruction
    manager->add(nextPC, BreakpointManager::kTypeSoftwareSingleStep);
    count++;
  }

  DS2LOG(Debug, "set %d single-step breakpoint(s) for Alpha", count);
  return count;
}

} // namespace Alpha
} // namespace Architecture
} // namespace ds2
