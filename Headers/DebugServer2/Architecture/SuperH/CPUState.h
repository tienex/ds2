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
namespace SuperH {

struct CPUState {
  // General Purpose Registers (32-bit)
  // r0-r7: general purpose
  // r8-r13: general purpose
  // r14: frame pointer (fp)
  // r15: stack pointer (sp)
  struct GPRegisterStruct {
    uint32_t regs[16]; // r0-r15
  } gp;

  // Special Registers
  struct SpecialRegisterStruct {
    uint32_t pc;      // Program Counter
    uint32_t pr;      // Procedure Register (return address)
    uint32_t sr;      // Status Register
    uint32_t gbr;     // Global Base Register
    uint32_t mach;    // Multiply-Accumulate High
    uint32_t macl;    // Multiply-Accumulate Low
    uint32_t vbr;     // Vector Base Register (interrupt/exception table)
  } special;

  // Floating-Point Registers (SH-4 and later)
  // Can be used as 16 double-precision or 32 single-precision
  // FR0-FR15 (bank 0), XF0-XF15 (bank 1)
  union FPRegisterFile {
    float s[32];      // Single precision (FR0-FR15, XF0-XF15)
    double d[16];     // Double precision (DR0-DR14, XD0-XD14)
    uint32_t w[32];   // Raw 32-bit values
  } fpu;

  // FPU Control and Status (SH-4)
  struct FPUControlStruct {
    uint32_t fpscr;   // FP Status/Control Register
    uint32_t fpul;    // FP Communication Register
  } fpu_ctrl;

  // DSP Registers (SH-DSP variants)
  struct DSPStruct {
    uint32_t a0;      // Accumulator A0
    uint32_t a0g;     // Accumulator A0 Guard
    uint32_t a1;      // Accumulator A1
    uint32_t a1g;     // Accumulator A1 Guard
    uint32_t x0;      // X register 0
    uint32_t x1;      // X register 1
    uint32_t y0;      // Y register 0
    uint32_t y1;      // Y register 1
    uint32_t m0;      // M register 0
    uint32_t m1;      // M register 1
    uint32_t dsr;     // DSP Status Register
  } dsp;

  // Control Registers (privileged)
  struct ControlStruct {
    uint32_t ssr;     // Saved Status Register
    uint32_t spc;     // Saved PC
    uint32_t sgr;     // Saved General Register 15
    uint32_t dbr;     // Debug Base Register
  } ctrl;

  // Banked Registers (8 sets of r0-r7 in some modes)
  struct BankedRegistersStruct {
    uint32_t r_bank[8]; // Banked r0-r7
  } banked;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper accessors
  inline uint32_t r0() const { return gp.regs[0]; }
  inline uint32_t sp() const { return gp.regs[15]; }
  inline uint32_t fp() const { return gp.regs[14]; }
  inline uint32_t pr() const { return special.pr; }
  inline uint32_t pc() const { return special.pc; }

  inline void setPC(uint32_t pc) { special.pc = pc; }
  inline void setStackPointer(uint32_t sp) { gp.regs[15] = sp; }
};
}
}
}
