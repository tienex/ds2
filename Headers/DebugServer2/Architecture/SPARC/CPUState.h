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

#if defined(ARCH_SPARC) || defined(ARCH_SPARC64)

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace SPARC {

//
// SPARC CPU State
//
// Supports both 32-bit SPARC (SPARC V8) and 64-bit SPARC (SPARC V9/UltraSPARC)
//

#if defined(ARCH_SPARC64)
using reg_t = uint64_t;
#else
using reg_t = uint32_t;
#endif

struct CPUState {
  // Global registers (g0-g7)
  union {
    reg_t gregs[8];
    struct {
      reg_t g0, g1, g2, g3, g4, g5, g6, g7;
    };
  } gp;

  // Output registers (o0-o7)
  union {
    reg_t oregs[8];
    struct {
      reg_t o0, o1, o2, o3, o4, o5, sp, o7;
    };
  } out;

  // Local registers (l0-l7)
  union {
    reg_t lregs[8];
    struct {
      reg_t l0, l1, l2, l3, l4, l5, l6, l7;
    };
  } local;

  // Input registers (i0-i7)
  union {
    reg_t iregs[8];
    struct {
      reg_t i0, i1, i2, i3, i4, i5, fp, i7;
    };
  } in;

  // Floating-point registers
  // SPARC V8: 32x32-bit registers, can be used as 16x64-bit or 8x128-bit (quad)
  // SPARC V9: 64 total FP registers
  union {
    float fregs[32];   // 32-bit floats
    double dregs[16];  // 64-bit doubles
    struct {
      float f0, f1, f2, f3, f4, f5, f6, f7;
      float f8, f9, f10, f11, f12, f13, f14, f15;
      float f16, f17, f18, f19, f20, f21, f22, f23;
      float f24, f25, f26, f27, f28, f29, f30, f31;
    };
  } fp;

  // Special purpose registers
  reg_t pc;     // Program counter
  reg_t npc;    // Next program counter
  reg_t y;      // Y register (multiply/divide)
  uint32_t psr; // Processor state register (V8) / PSTATE (V9)
  uint32_t wim; // Window invalid mask (V8)
  uint32_t tbr; // Trap base register (V8)
  uint64_t fsr; // Floating-point state register
  uint64_t fprs;// FP registers state (V9)
  uint64_t ccr; // Condition codes register (V9)
  uint64_t asi; // Address space identifier (V9)

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

} // namespace SPARC
} // namespace Architecture
} // namespace ds2

#endif // ARCH_SPARC || ARCH_SPARC64
