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

#include "DebugServer2/Architecture/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace ROMP {

// CPU State for IBM ROMP (RT PC)
// ROMP = Research/Office Products Microprocessor
// IBM's first commercial RISC processor (1986)
// 32-bit RISC architecture, precursor to IBM POWER

struct CPUState {
  // General purpose registers (32-bit)
  union {
    struct {
      uint32_t r0;   // Register 0 (often used as scratch)
      uint32_t r1;   // Register 1
      uint32_t r2;   // Register 2
      uint32_t r3;   // Register 3
      uint32_t r4;   // Register 4
      uint32_t r5;   // Register 5
      uint32_t r6;   // Register 6
      uint32_t r7;   // Register 7
      uint32_t r8;   // Register 8
      uint32_t r9;   // Register 9
      uint32_t r10;  // Register 10
      uint32_t r11;  // Register 11
      uint32_t r12;  // Register 12
      uint32_t r13;  // Register 13
      uint32_t r14;  // Stack pointer (SP)
      uint32_t r15;  // Frame pointer / Link register
    };
    uint32_t r[16];
  };

  // Program Counter
  uint32_t pc;

  // Instruction Address Register (IAR) - next instruction
  uint32_t iar;

  // Condition Code Register (CCR)
  uint32_t ccr;

  // Machine Check Interrupt Register (MCIR)
  uint32_t mcir;

  // Interrupt Control Registers
  uint32_t ics;       // Interrupt Control/Status
  uint32_t cs;        // Control/Status register

  // Memory Management Unit (MMU) registers
  struct {
    uint32_t seg[16];  // Segment registers (16 segments)
    uint32_t tcr;      // Translation Control Register
    uint32_t tid;      // Translation ID
  } mmu;

  // Floating Point Unit registers (optional on ROMP)
  struct {
    double fpr[4];     // FP registers (ROMP has limited FPU)
    uint32_t fpscr;    // FP Status and Control Register
  } fpu;

  CPUState() { memset(this, 0, sizeof(*this)); }

  // Condition Code Register (CCR) bits
  enum CCRBits {
    kCCR_LT = (1 << 0),   // Less Than
    kCCR_GT = (1 << 1),   // Greater Than
    kCCR_EQ = (1 << 2),   // Equal
    kCCR_OV = (1 << 3),   // Overflow
    kCCR_CA = (1 << 4),   // Carry
    kCCR_TB = (1 << 5),   // Test Bit
  };

  // Control/Status Register bits
  enum CSBits {
    kCS_TE = (1 << 0),    // Trace Enable
    kCS_EXC = (1 << 1),   // Exception occurred
    kCS_IO = (1 << 2),    // I/O mode
  };

  // Get PC
  inline uint32_t getPC() const {
    return pc;
  }

  // Set PC
  inline void setPC(uint32_t addr) {
    pc = addr;
    iar = addr; // Update IAR as well
  }

  // Get SP (stack pointer)
  inline uint32_t getSP() const {
    return r14;
  }

  // Set SP
  inline void setSP(uint32_t addr) {
    r14 = addr;
  }

  // Get return address (link register)
  inline uint32_t retaddr() const {
    return r15;
  }

  // Single step support
  inline void setSingleStep(bool enable) {
    if (enable) {
      cs |= kCS_TE;
    } else {
      cs &= ~kCS_TE;
    }
  }

  inline bool isSingleStep() const {
    return (cs & kCS_TE) != 0;
  }

  // Check condition codes
  inline bool isZero() const {
    return (ccr & kCCR_EQ) != 0;
  }

  inline bool isNegative() const {
    return (ccr & kCCR_LT) != 0;
  }

  inline bool isCarry() const {
    return (ccr & kCCR_CA) != 0;
  }

  inline bool isOverflow() const {
    return (ccr & kCCR_OV) != 0;
  }
};

// Register numbering for GDB protocol
enum GDBRegisterIndex {
  kGDBRegisterR0 = 0,
  kGDBRegisterR1 = 1,
  kGDBRegisterR2 = 2,
  kGDBRegisterR3 = 3,
  kGDBRegisterR4 = 4,
  kGDBRegisterR5 = 5,
  kGDBRegisterR6 = 6,
  kGDBRegisterR7 = 7,
  kGDBRegisterR8 = 8,
  kGDBRegisterR9 = 9,
  kGDBRegisterR10 = 10,
  kGDBRegisterR11 = 11,
  kGDBRegisterR12 = 12,
  kGDBRegisterR13 = 13,
  kGDBRegisterSP = 14,
  kGDBRegisterFP = 15,
  kGDBRegisterPC = 16,
  kGDBRegisterCCR = 17,
  kGDBRegisterCount = 18
};

} // namespace ROMP
} // namespace Architecture
} // namespace ds2
