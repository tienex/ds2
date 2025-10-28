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
namespace LoongArch64 {

struct CPUState {
  // General Purpose Registers (64-bit)
  // r0 is hardwired to zero
  // r1 = ra (return address)
  // r2 = tp (thread pointer)
  // r3 = sp (stack pointer)
  // r22 = fp (frame pointer)
  struct GPRegisterStruct {
    uint64_t regs[32]; // r0-r31
  } gp;

  // Special Registers
  struct SpecialRegisterStruct {
    uint64_t pc;      // Program Counter (CSR.ERA)
    uint64_t badv;    // Bad Virtual Address (CSR.BADV)
    uint64_t badi;    // Bad Instruction (CSR.BADI)
  } special;

  // Floating-Point Registers (32 x 64-bit, can hold double)
  // In LoongArch64, FP registers are always 64-bit
  union FPRegisterFile {
    float s[32];      // Single precision view
    double d[32];     // Double precision (native size)
  } fpu;

  // FPU Control and Status
  struct FPUControlStruct {
    uint32_t fcsr;    // FP Control and Status Register
    uint64_t fcc;     // FP Condition Code (8 condition flags)
  } fpu_ctrl;

  // LoongArch Configuration Registers (read-only in user space)
  struct ConfigStruct {
    uint32_t cpucfg[16]; // CPU configuration registers
  } config;

  // LSX (Loongson SIMD Extension) - 128-bit vector registers
  // LSX provides 32 x 128-bit vector registers (v0-v31)
  struct LSXRegisterStruct {
    uint8_t v[32][16]; // 32 x 128-bit registers
  } lsx;

  // LASX (Loongson Advanced SIMD Extension) - 256-bit vector registers
  // LASX provides 32 x 256-bit vector registers (xv0-xv31)
  struct LASXRegisterStruct {
    uint8_t xv[32][32]; // 32 x 256-bit registers
  } lasx;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper for zero register
  inline uint64_t r0() const { return 0; }
  inline uint64_t ra() const { return gp.regs[1]; }
  inline uint64_t tp() const { return gp.regs[2]; }
  inline uint64_t sp() const { return gp.regs[3]; }
  inline uint64_t fp() const { return gp.regs[22]; }
  inline uint64_t pc() const { return special.pc; }

  inline void setPC(uint64_t pc) { special.pc = pc; }
  inline void setStackPointer(uint64_t sp) { gp.regs[3] = sp; }
};
}
}
}
