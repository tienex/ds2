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
namespace SPARC64 {

//
// SPARC64 (SPARCv9) 64-bit CPU State
// 64-bit RISC architecture by Sun Microsystems (1995-present)
// Used in: Sun/Oracle servers, Fujitsu SPARC64 systems
//

struct CPUState {
  //
  // General Purpose Registers (32 x 64-bit)
  // Register windows with 64-bit extension
  //
  struct {
    union {
      uint64_t regs[32];
      struct {
        uint64_t g0, g1, g2, g3, g4, g5, g6, g7;  // Global
        uint64_t o0, o1, o2, o3, o4, o5, o6, o7;  // Out
        uint64_t l0, l1, l2, l3, l4, l5, l6, l7;  // Local
        uint64_t i0, i1, i2, i3, i4, i5, i6, i7;  // In
      };
    };
  } gp;

  struct {
    uint64_t pc;      // Program Counter
    uint64_t npc;     // Next Program Counter
    uint64_t y;       // Multiply/Divide register (32-bit)
  } special;

  struct {
    uint64_t cwp : 5;    // Current Window Pointer
    uint64_t pstate;     // Processor State
    uint64_t tstate;     // Trap State
    uint64_t tt : 9;     // Trap Type
    uint64_t tl : 3;     // Trap Level
    uint64_t pil : 4;    // Processor Interrupt Level
  } state;

  uint64_t ccr;          // Condition Code Register
  uint64_t asi;          // Address Space Identifier
  uint64_t fprs;         // FP Register State

  struct {
    uint64_t tba;        // Trap Base Address
    uint64_t tpc[6];     // Trap PC (per trap level)
    uint64_t tnpc[6];    // Trap NPC (per trap level)
    uint64_t tstate[6];  // Trap State (per trap level)
    uint64_t tt[6];      // Trap Type (per trap level)
  } trap;

  union {
    float s[32];         // Single-precision (32 regs)
    double d[32];        // Double-precision (32 regs in v9)
    struct {
      uint64_t low;
      uint64_t high;
    } q[16];             // Quad-precision (16 regs)
    uint32_t raw32[64];  // Raw 32-bit access
    uint64_t raw64[32];  // Raw 64-bit access
  } fpu;

  struct {
    uint64_t cexc : 5; uint64_t aexc : 5; uint64_t fcc0 : 2;
    uint64_t qne : 1; uint64_t ftt : 3; uint64_t ver : 3;
    uint64_t reserved1 : 2; uint64_t tem : 5; uint64_t ns : 1;
    uint64_t rd : 2; uint64_t fcc1 : 2; uint64_t fcc2 : 2;
    uint64_t fcc3 : 2; uint64_t reserved2 : 33;
  } fsr;

  uint64_t gsr;          // Graphics Status Register (VIS)
  uint64_t tick;         // Tick register
  uint64_t canrestore;   // Restorable windows
  uint64_t cansave;      // Savable windows
  uint64_t cleanwin;     // Clean windows
  uint64_t otherwin;     // Other windows
  uint64_t wstate;       // Window State
};

} // namespace SPARC64
} // namespace Architecture
} // namespace ds2
