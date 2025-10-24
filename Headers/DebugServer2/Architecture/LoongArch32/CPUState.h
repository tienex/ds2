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
namespace LoongArch32 {

struct CPUState {
  // General Purpose Registers (32-bit)
  // r0 is hardwired to zero
  // r1 = ra (return address)
  // r2 = tp (thread pointer)
  // r3 = sp (stack pointer)
  // r22 = fp (frame pointer)
  struct GPRegisterStruct {
    uint32_t regs[32]; // r0-r31
  } gp;

  // Special Registers
  struct SpecialRegisterStruct {
    uint32_t pc;      // Program Counter
    uint32_t badv;    // Bad Virtual Address
    uint32_t badi;    // Bad Instruction
  } special;

  // Floating-Point Registers (32 x 32-bit)
  union FPRegisterFile {
    float s[32];      // Single precision (32-bit)
    double d[16];     // Double precision (64-bit, using pairs)
  } fpu;

  // FPU Control and Status
  struct FPUControlStruct {
    uint32_t fcsr;    // FP Control and Status Register
    uint32_t fcc;     // FP Condition Code
  } fpu_ctrl;

  // LoongArch Configuration Registers (read-only in user space)
  struct ConfigStruct {
    uint32_t cpucfg[16]; // CPU configuration registers
  } config;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper for zero register
  inline uint32_t r0() const { return 0; }
  inline uint32_t ra() const { return gp.regs[1]; }
  inline uint32_t tp() const { return gp.regs[2]; }
  inline uint32_t sp() const { return gp.regs[3]; }
  inline uint32_t fp() const { return gp.regs[22]; }
  inline uint32_t pc() const { return special.pc; }

  inline void setPC(uint32_t pc) { special.pc = pc; }
  inline void setStackPointer(uint32_t sp) { gp.regs[3] = sp; }
};
}
}
}
