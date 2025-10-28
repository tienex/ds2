//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Architecture {
namespace MIPS {

//
// MIPS Instruction Set Modes
// Similar to ARM's Thumb/ARM modes, MIPS can switch between different
// instruction encodings for code compression and performance
//

enum InstructionMode {
  kInstructionModeNormal = 0,   // Standard MIPS32/MIPS64 instructions (32-bit)
  kInstructionModeMIPS16 = 1,   // MIPS16e compressed instructions (16-bit)
  kInstructionModeMicroMIPS = 2 // microMIPS compressed instructions (16/32-bit)
};

//
// MIPS ISA Mode bit in PC/RA
// The LSB of PC indicates the current instruction mode:
// - Bit 0 = 0: Normal MIPS mode
// - Bit 0 = 1: microMIPS or MIPS16 mode (depending on CPU configuration)
//

static inline bool IsMIPSCompressedMode(uint64_t pc) {
  return (pc & 0x1) != 0;
}

static inline uint64_t GetMIPSInstructionAddress(uint64_t pc) {
  // Clear the ISA mode bit to get the actual instruction address
  return pc & ~0x1ULL;
}

static inline uint64_t SetMIPSCompressedMode(uint64_t pc, bool compressed) {
  pc = GetMIPSInstructionAddress(pc);
  if (compressed) {
    pc |= 0x1;
  }
  return pc;
}

//
// Instruction size helpers
//

static inline size_t GetMIPSInstructionSize(InstructionMode mode,
                                             uint16_t firstHalfword) {
  switch (mode) {
  case kInstructionModeNormal:
    return 4; // All standard MIPS instructions are 32-bit

  case kInstructionModeMIPS16:
    // MIPS16e: Most instructions are 16-bit, some are 32-bit
    // Extended instructions have specific patterns in the opcode
    if ((firstHalfword & 0xF800) == 0xF000) {
      // EXTEND instruction - next instruction is extended
      return 4;
    }
    return 2;

  case kInstructionModeMicroMIPS:
    // microMIPS: 16-bit or 32-bit instructions
    // 32-bit instructions have major opcode in specific ranges
    uint16_t majorOpcode = (firstHalfword >> 10) & 0x3F;
    if (majorOpcode >= 0x10) {
      // 32-bit instruction
      return 4;
    }
    // Check for specific 32-bit opcodes in lower range
    if (majorOpcode == 0x00 || majorOpcode == 0x01 || majorOpcode == 0x04 ||
        majorOpcode == 0x05 || majorOpcode == 0x0C || majorOpcode == 0x0D) {
      return 4;
    }
    return 2;
  }

  return 4; // Default to 32-bit
}

//
// ISA Mode Selection
// The ISA mode can be determined from:
// 1. The LSB of the target address in jump/branch instructions
// 2. The Config3.ISAOnExc bit (microMIPS on exception)
// 3. Special instructions (JALX switches between MIPS and MIPS16)
//

struct InstructionModeInfo {
  InstructionMode mode;
  bool canSwitchMode;        // Can this instruction switch modes?
  InstructionMode targetMode; // Target mode if switching
};

//
// Mode switching instructions
//

static inline bool IsModeSwitch(uint32_t instruction,
                                 InstructionModeInfo &info) {
  info.canSwitchMode = false;

  // In normal MIPS mode, check for JALX (Jump and Link Exchange)
  if ((instruction >> 26) == 0x1D) { // JALX opcode
    info.canSwitchMode = true;
    info.targetMode = kInstructionModeMIPS16; // or microMIPS depending on CPU
    return true;
  }

  return false;
}

//
// CP0 Status/Config register bits related to ISA mode
//

enum {
  // Config3 register
  MIPS_CONFIG3_ISA_ON_EXC = (1 << 16), // Use microMIPS on exceptions
  MIPS_CONFIG3_ISA_ON_EXC_SHIFT = 16,

  // Status register
  MIPS_STATUS_ISA_MODE = (1 << 26), // Current ISA mode indicator (some CPUs)
  MIPS_STATUS_ISA_MODE_SHIFT = 26
};

} // namespace MIPS
} // namespace Architecture
} // namespace ds2
