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

#if !defined(CPUSTATE_H_INTERNAL)
#error "You shall not include this file directly."
#endif

#include "DebugServer2/Architecture/Alpha/RegistersDescriptors.h"

#include <cstring>

namespace ds2 {
namespace Architecture {
namespace Alpha {

#pragma pack(push, 1)

//
// Alpha CPU State
//
// This structure represents the complete state of an Alpha CPU.
// Alpha is a 64-bit RISC architecture developed by DEC.
//
// Register Conventions:
//   Integer Registers:
//     r0         - Function result / caller-saved
//     r1-r8      - Caller-saved temporaries
//     r9-r15     - Callee-saved
//     r16-r21    - Argument registers (a0-a5)
//     r22-r25    - Caller-saved temporaries (t8-t11)
//     r26 (ra)   - Return address
//     r27 (pv)   - Procedure value
//     r28 (at)   - Assembler temporary
//     r29 (gp)   - Global pointer
//     r30 (sp)   - Stack pointer
//     r31 (zero) - Hardwired to 0
//
//   Floating-Point Registers:
//     f0, f1     - Function results
//     f2-f9      - Callee-saved
//     f10-f15    - Caller-saved temporaries
//     f16-f21    - Argument registers
//     f22-f30    - Caller-saved temporaries
//     f31 (fzero)- Hardwired to 0.0
//

struct CPUState {
  union {
    uint64_t regs[32];
    struct {
      uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
      uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
      uint64_t r16, r17, r18, r19, r20, r21, r22, r23;
      uint64_t r24, r25, r26, r27, r28, r29, r30, r31;
    };
    struct {
      uint64_t v0, t0, t1, t2, t3, t4, t5, t6;
      uint64_t t7, s0, s1, s2, s3, s4, s5, fp;
      uint64_t a0, a1, a2, a3, a4, a5, t8, t9;
      uint64_t t10, t11, ra, pv, at, gp, sp, zero;
    };
  } gp;

  // Floating-point registers (64-bit IEEE 754)
  union {
    uint64_t raw[32];
    double regs[32];
    struct {
      double f0, f1, f2, f3, f4, f5, f6, f7;
      double f8, f9, f10, f11, f12, f13, f14, f15;
      double f16, f17, f18, f19, f20, f21, f22, f23;
      double f24, f25, f26, f27, f28, f29, f30, f31;
    };
  } fp;

  uint64_t pc;       // Program counter
  uint64_t fpcr;     // Floating-point control register
  uint64_t unique;   // Unique value (thread pointer on Linux)

  CPUState() { clear(); }

  void clear() { memset(this, 0, sizeof(*this)); }

  inline uint64_t pc() const { return pc; }
  inline void setPC(uint64_t val) { pc = val; }

  inline uint64_t sp() const { return gp.sp; }
  inline void setSP(uint64_t val) { gp.sp = val; }

  inline uint64_t retval() const { return gp.v0; }
};

#pragma pack(pop)

} // namespace Alpha
} // namespace Architecture
} // namespace ds2
