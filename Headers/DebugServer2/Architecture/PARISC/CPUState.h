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
namespace PARISC {

// PA-RISC 1.0 and 1.1 (32-bit)
// HP Precision Architecture RISC
// Used in HP 9000 workstations and servers

struct CPUState {
  // General Purpose Registers (32-bit)
  // gr0 is hardwired to zero
  // gr1 is typically used as a temporary
  // gr2 is the return pointer (rp)
  // gr27 is the data pointer (dp)
  // gr30 is the stack pointer (sp)
  // gr31 is used for millicode return
  struct GPRegisterStruct {
    uint32_t regs[32]; // gr0-gr31
  } gp;

  // Space Registers (segmented addressing)
  // sr0-sr3 are used for different address spaces
  // sr4-sr7 are typically used for user space
  struct SpaceRegisterStruct {
    uint32_t regs[8]; // sr0-sr7
  } sr;

  // Special Registers
  struct SpecialRegisterStruct {
    uint32_t iaoq_head; // Instruction Address Offset Queue (head)
    uint32_t iaoq_tail; // Instruction Address Offset Queue (tail)
    uint32_t iasq_head; // Instruction Address Space Queue (head)
    uint32_t iasq_tail; // Instruction Address Space Queue (tail)
    uint32_t ipsw;      // Interrupt PSW
    uint32_t psw;       // Program Status Word
    uint32_t sar;       // Shift Amount Register
    uint32_t pcoq_head; // PC Offset Queue (head)
    uint32_t pcoq_tail; // PC Offset Queue (tail)
    uint32_t pcsq_head; // PC Space Queue (head)
    uint32_t pcsq_tail; // PC Space Queue (tail)
    uint32_t eiem;      // External Interrupt Enable Mask
    uint32_t iir;       // Interrupt Instruction Register
    uint32_t isr;       // Interrupt Space Register
    uint32_t ior;       // Interrupt Offset Register
    uint32_t iva;       // Interrupt Vector Address
    uint32_t rctr;      // Recovery Counter
  } special;

  // Control Registers
  struct ControlRegisterStruct {
    uint32_t cr[32]; // cr0-cr31 (various control functions)
  } cr;

  // Floating-Point Registers (64-bit double precision)
  // PA-RISC FPU has 32 double-precision registers
  // Can also be accessed as 64 single-precision
  union FPRegisterFile {
    double d[32];   // Double precision (native)
    float s[64];    // Single precision (paired)
    uint64_t l[32]; // Raw 64-bit values
  } fpu;

  // FPU Status
  struct FPUStatusStruct {
    uint32_t fpsr; // FP Status Register
  } fpu_status;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper accessors
  inline uint32_t gr0() const { return 0; } // Always zero
  inline uint32_t rp() const { return gp.regs[2]; }
  inline uint32_t sp() const { return gp.regs[30]; }
  inline uint32_t dp() const { return gp.regs[27]; }
  inline uint32_t pc() const { return special.iaoq_head; }

  inline void setPC(uint32_t pc) {
    special.iaoq_head = pc;
    special.iaoq_tail = pc + 4;
  }

  inline void setStackPointer(uint32_t sp) { gp.regs[30] = sp; }
};

} // namespace PARISC
} // namespace Architecture
} // namespace ds2
