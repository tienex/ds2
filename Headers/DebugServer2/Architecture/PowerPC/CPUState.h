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

#if defined(ARCH_PPC) || defined(ARCH_PPC64)

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {

//
// PowerPC CPU State
//
// Supports both 32-bit PowerPC and 64-bit PowerPC
//

#if defined(ARCH_PPC64)
using reg_t = uint64_t;
#else
using reg_t = uint32_t;
#endif

struct CPUState {
  // General purpose registers (r0-r31)
  union {
    reg_t regs[32];
    struct {
      reg_t r0, r1, r2, r3, r4, r5, r6, r7;
      reg_t r8, r9, r10, r11, r12, r13, r14, r15;
      reg_t r16, r17, r18, r19, r20, r21, r22, r23;
      reg_t r24, r25, r26, r27, r28, r29, r30, r31;
    };
    struct {
      reg_t gpr0, sp, toc, r3_arg0, r4_arg1, r5_arg2, r6_arg3, r7_arg4;
      reg_t r8_arg5, r9_arg6, r10_arg7, r11, r12, r13, r14, r15;
      reg_t r16, r17, r18, r19, r20, r21, r22, r23;
      reg_t r24, r25, r26, r27, r28, r29, r30, r31;
    };
  } gp;

  // Floating-point registers (f0-f31, 64-bit IEEE 754)
  union {
    double regs[32];
    struct {
      double f0, f1, f2, f3, f4, f5, f6, f7;
      double f8, f9, f10, f11, f12, f13, f14, f15;
      double f16, f17, f18, f19, f20, f21, f22, f23;
      double f24, f25, f26, f27, f28, f29, f30, f31;
    };
  } fp;

  // Vector registers (AltiVec/VMX, 128-bit) - PowerPC G4 (7400) and later
  struct {
    uint32_t v[4]; // 128-bit vector (quad-word)
  } vr[32];

  // VSX registers (Vector-Scalar Extension) - POWER7 and later
  // VSX provides vs0-vs63 where vs0-vs31 overlay fp0-fp31 and vr0-vr31
  struct {
    uint64_t dw[2]; // 128-bit as two 64-bit doublewords
  } vsx[64];

  // Special purpose registers
  reg_t pc;      // Program counter (NIP/IAR - Next Instruction Pointer / Instruction Address Register)
  reg_t msr;     // Machine state register
  uint32_t cr;   // Condition register (8 x 4-bit fields)
  reg_t lr;      // Link register
  reg_t ctr;     // Count register
  reg_t xer;     // Fixed-point exception register

  // Additional special purpose registers (availability varies by CPU)
  reg_t dar;     // Data address register (for exceptions)
  reg_t dsisr;   // Data storage interrupt status register
  reg_t srr0;    // Save/restore register 0 (return address)
  reg_t srr1;    // Save/restore register 1 (MSR save)

  // Floating-point status and control register
  uint32_t fpscr;

  // Vector status and control register (AltiVec/VMX) - G4 and later
  uint32_t vscr;
  uint32_t vrsave;

  // Segment registers (32-bit PowerPC only)
#if !defined(ARCH_PPC64)
  uint32_t sr[16]; // Segment registers sr0-sr15
#endif

  // Time base registers (read-only in user mode)
  uint64_t tb;   // Time base (64-bit counter)

  // Processor version register (read-only)
  uint32_t pvr;  // Processor version register

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

} // namespace PowerPC
} // namespace Architecture
} // namespace ds2

#endif // ARCH_PPC || ARCH_PPC64
