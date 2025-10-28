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
namespace RISCV64 {

struct CPUState {
  // General Purpose Registers (64-bit)
  // x0 is hardwired to zero
  // x1 = ra (return address)
  // x2 = sp (stack pointer)
  // x3 = gp (global pointer)
  // x4 = tp (thread pointer)
  // x5-x7 = t0-t2 (temporaries)
  // x8 = s0/fp (saved register / frame pointer)
  // x9 = s1 (saved register)
  // x10-x11 = a0-a1 (function arguments / return values)
  // x12-x17 = a2-a7 (function arguments)
  // x18-x27 = s2-s11 (saved registers)
  // x28-x31 = t3-t6 (temporaries)
  struct GPRegisterStruct {
    uint64_t regs[32]; // x0-x31
  } gp;

  // Special Registers
  struct SpecialRegisterStruct {
    uint64_t pc;      // Program Counter
  } special;

  // Floating-Point Registers (F and D extensions)
  // F extension: 32 x 32-bit single-precision
  // D extension: 32 x 64-bit double-precision
  // Q extension: 32 x 128-bit quad-precision (future)
  union FPRegisterFile {
    float s[32];      // Single precision (F extension)
    double d[32];     // Double precision (D extension)
    uint64_t l[32];   // Raw 64-bit values
  } fpu;

  // FPU Control and Status
  struct FPUControlStruct {
    uint64_t fcsr;    // FP Control and Status Register
    uint32_t frm;     // FP Rounding Mode (bits 5-7 of fcsr)
    uint32_t fflags;  // FP Exception Flags (bits 0-4 of fcsr)
  } fpu_ctrl;

  // CSRs (Control and Status Registers) - user mode readable
  struct CSRStruct {
    uint64_t cycle;     // Cycle counter
    uint64_t time;      // Timer
    uint64_t instret;   // Instructions retired
  } csr;

  // Vector Extension (V extension) - optional
  // Provides variable-length vectors up to VLEN bits
  struct VectorStruct {
    uint8_t v[32][256]; // 32 vector registers (max 2048 bits each)
    uint64_t vl;        // Vector length
    uint64_t vtype;     // Vector type
    uint64_t vstart;    // Vector start index
  } vec;

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  // Helper accessors
  inline uint64_t x0() const { return 0; }
  inline uint64_t ra() const { return gp.regs[1]; }
  inline uint64_t sp() const { return gp.regs[2]; }
  inline uint64_t gp_reg() const { return gp.regs[3]; }
  inline uint64_t tp() const { return gp.regs[4]; }
  inline uint64_t fp() const { return gp.regs[8]; }
  inline uint64_t pc() const { return special.pc; }

  inline void setPC(uint64_t pc) { special.pc = pc; }
  inline void setStackPointer(uint64_t sp) { gp.regs[2] = sp; }
};
}
}
}
