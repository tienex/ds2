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
namespace PARISC64 {

// PA-RISC 2.0 (64-bit)
// HP Precision Architecture 64-bit (PA-8000 and later)
// Also known as PA-RISC 64 or PA-2.0

struct CPUState {
  // General Purpose Registers (64-bit)
  // gr0 is hardwired to zero
  // gr1 is typically used as a temporary
  // gr2 is the return pointer (rp)
  // gr27 is the data pointer (dp)
  // gr30 is the stack pointer (sp)
  // gr31 is used for millicode return
  struct GPRegisterStruct {
    uint64_t regs[32]; // gr0-gr31
  } gp;

  // Space Registers (64-bit in PA-RISC 2.0)
  // sr0-sr3 are used for different address spaces
  // sr4-sr7 are typically used for user space
  struct SpaceRegisterStruct {
    uint64_t regs[8]; // sr0-sr7
  } sr;

  // Special Registers (64-bit)
  struct SpecialRegisterStruct {
    uint64_t iaoq_head; // Instruction Address Offset Queue (head)
    uint64_t iaoq_tail; // Instruction Address Offset Queue (tail)
    uint64_t iasq_head; // Instruction Address Space Queue (head)
    uint64_t iasq_tail; // Instruction Address Space Queue (tail)
    uint64_t ipsw;      // Interrupt PSW
    uint64_t psw;       // Program Status Word
    uint64_t sar;       // Shift Amount Register
    uint64_t pcoq_head; // PC Offset Queue (head)
    uint64_t pcoq_tail; // PC Offset Queue (tail)
    uint64_t pcsq_head; // PC Space Queue (head)
    uint64_t pcsq_tail; // PC Space Queue (tail)
    uint64_t eiem;      // External Interrupt Enable Mask
    uint64_t iir;       // Interrupt Instruction Register
    uint64_t isr;       // Interrupt Space Register
    uint64_t ior;       // Interrupt Offset Register
    uint64_t iva;       // Interrupt Vector Address
    uint64_t rctr;      // Recovery Counter
  } special;

  // Control Registers (64-bit)
  struct ControlRegisterStruct {
    uint64_t cr[32]; // cr0-cr31 (various control functions)
  } cr;

  // Floating-Point Registers (64-bit double precision)
  // PA-RISC 2.0 FPU has 32 double-precision registers
  // Can also be accessed as 64 single-precision
  union FPRegisterFile {
    double d[32];   // Double precision (native)
    float s[64];    // Single precision (paired)
    uint64_t l[32]; // Raw 64-bit values
  } fpu;

  // FPU Status (64-bit)
  struct FPUStatusStruct {
    uint64_t fpsr; // FP Status Register
  } fpu_status;

  // Performance Monitor Registers (PA-RISC 2.0 feature)
  struct PerformanceMonitorStruct {
    uint64_t pmc[4];  // Performance Monitor Counters
    uint64_t pmd[4];  // Performance Monitor Data
  } perf;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper accessors
  inline uint64_t gr0() const { return 0; } // Always zero
  inline uint64_t rp() const { return gp.regs[2]; }
  inline uint64_t sp() const { return gp.regs[30]; }
  inline uint64_t dp() const { return gp.regs[27]; }
  inline uint64_t pc() const { return special.iaoq_head; }

  inline void setPC(uint64_t pc) {
    special.iaoq_head = pc;
    special.iaoq_tail = pc + 4;
  }

  inline void setStackPointer(uint64_t sp) { gp.regs[30] = sp; }
};

} // namespace PARISC64
} // namespace Architecture
} // namespace ds2
