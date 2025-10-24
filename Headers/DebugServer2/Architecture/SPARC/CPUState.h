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
namespace SPARC {

//
// SPARC (Scalable Processor ARChitecture) 32-bit CPU State
// RISC architecture by Sun Microsystems (1987-present)
// Used in: Sun workstations/servers, embedded systems
// Variants: SPARC v7, v8, v8E (with FPU)
//

struct CPUState {
  //
  // General Purpose Registers (32 x 32-bit)
  // SPARC uses register windows for efficient procedure calls
  //
  struct {
    union {
      uint32_t regs[32];
      struct {
        uint32_t g0, g1, g2, g3, g4, g5, g6, g7;  // Global (g0 = 0)
        uint32_t o0, o1, o2, o3, o4, o5, o6, o7;  // Out (o6=sp, o7=retaddr-8)
        uint32_t l0, l1, l2, l3, l4, l5, l6, l7;  // Local
        uint32_t i0, i1, i2, i3, i4, i5, i6, i7;  // In (i6=fp, i7=retaddr)
      };
    };
  } gp;

  struct {
    uint32_t pc;      // Program Counter
    uint32_t npc;     // Next Program Counter (delayed branch)
    uint32_t y;       // Multiply/Divide register
  } special;

  struct {
    uint32_t cwp : 5;    // Current Window Pointer
    uint32_t et : 1;     // Enable Traps
    uint32_t ps : 1;     // Previous Supervisor
    uint32_t s : 1;      // Supervisor mode
    uint32_t pil : 4;    // Processor Interrupt Level
    uint32_t ef : 1;     // Enable Floating-point
    uint32_t ec : 1;     // Enable Coprocessor
    uint32_t reserved : 6;
    uint32_t icc : 4;    // Integer Condition Codes
    uint32_t ver : 4;    // Version
    uint32_t impl : 4;   // Implementation
  } psr;

  uint32_t wim;          // Window Invalid Mask

  struct {
    uint32_t tba : 20;   // Trap Base Address
    uint32_t tt : 8;     // Trap Type
    uint32_t zero : 4;
  } tbr;

  union {
    float s[32];         // Single-precision
    double d[16];        // Double-precision
    uint32_t raw32[32];
    uint64_t raw64[16];
  } fpu;

  struct {
    uint32_t cexc : 5; uint32_t aexc : 5; uint32_t fcc : 2;
    uint32_t qne : 1; uint32_t reserved1 : 2; uint32_t ftt : 3;
    uint32_t ver : 3; uint32_t reserved2 : 2; uint32_t tem : 5;
    uint32_t ns : 1; uint32_t rd : 2; uint32_t reserved3 : 1;
  } fsr;

  uint32_t asr[32];      // Ancillary State Registers
};

} // namespace SPARC
} // namespace Architecture
} // namespace ds2
