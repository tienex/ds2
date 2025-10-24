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

#if defined(ARCH_PARISC) || defined(ARCH_PARISC64)

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PARISC {

//
// PA-RISC CPU State
//
// Supports both 32-bit PA-RISC and 64-bit PA-RISC (PA-RISC 2.0)
//

#if defined(ARCH_PARISC64)
using reg_t = uint64_t;
#else
using reg_t = uint32_t;
#endif

struct CPUState {
  // General purpose registers (gr0-gr31)
  union {
    reg_t regs[32];
    struct {
      reg_t gr0, gr1, gr2, gr3, gr4, gr5, gr6, gr7;
      reg_t gr8, gr9, gr10, gr11, gr12, gr13, gr14, gr15;
      reg_t gr16, gr17, gr18, gr19, gr20, gr21, gr22, gr23;
      reg_t gr24, gr25, gr26, gr27, gr28, gr29, gr30, gr31;
    };
    struct {
      reg_t zero, r1, rp, r3, r4, r5, r6, r7;
      reg_t r8, r9, r10, r11, r12, r13, r14, r15;
      reg_t r16, r17, r18, r19, r20, r21, r22, arg3;
      reg_t arg2, arg1, arg0, dp, r28, r29, sp, r31;
    };
  } gp;

  // Floating-point registers (fr0-fr31, 64-bit IEEE 754)
  union {
    double regs[32];
    struct {
      double fr0, fr1, fr2, fr3, fr4, fr5, fr6, fr7;
      double fr8, fr9, fr10, fr11, fr12, fr13, fr14, fr15;
      double fr16, fr17, fr18, fr19, fr20, fr21, fr22, fr23;
      double fr24, fr25, fr26, fr27, fr28, fr29, fr30, fr31;
    };
  } fp;

  // Special purpose registers
  reg_t iaoq_head;  // Instruction address offset queue head (PC)
  reg_t iaoq_tail;  // Instruction address offset queue tail (PC+4)
  reg_t iasq_head;  // Instruction address space queue head
  reg_t iasq_tail;  // Instruction address space queue tail
  reg_t sar;        // Shift amount register
  reg_t pcoq_head;  // PC offset queue head (legacy name)
  reg_t pcoq_tail;  // PC offset queue tail (legacy name)
  reg_t pcsq_head;  // PC space queue head (legacy name)
  reg_t pcsq_tail;  // PC space queue tail (legacy name)
  reg_t cr0;        // Recovery counter
  reg_t cr24;       // Kernel register
  reg_t cr25;       // Low kernel register
  reg_t cr26;       // High kernel register
  reg_t cr27;       // Thread pointer
  reg_t cr28;       // Shadow registers

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

} // namespace PARISC
} // namespace Architecture
} // namespace ds2

#endif // ARCH_PARISC || ARCH_PARISC64
