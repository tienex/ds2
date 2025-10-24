//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Xbox 360 Xenon VMX128 Extension
//
// VMX128 is Microsoft's extension to AltiVec/VMX for the Xbox 360 (Xenon).
// Key differences from standard AltiVec:
// - 128 vector registers (v0-v127) instead of 32
// - Additional SIMD instructions
// - No VRSAVE register
// - Some instruction behavior differences
//

#pragma once

#include "DebugServer2/Architecture/PowerPC/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace Xenon {

// Xbox 360 Xenon CPU state with VMX128 extension
struct VMX128State {
  // General purpose registers (r0-r31, 64-bit even on Xenon)
  union {
    uint64_t regs[32];
    struct {
      uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
      uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
      uint64_t r16, r17, r18, r19, r20, r21, r22, r23;
      uint64_t r24, r25, r26, r27, r28, r29, r30, r31;
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

  // VMX128 vector registers (v0-v127, 128-bit each)
  // This is the Xenon extension - standard AltiVec only has v0-v31
  struct {
    uint32_t v[4]; // 128-bit vector as 4 x 32-bit words
  } vr[128];

  // Special purpose registers
  uint64_t pc;      // Program counter (NIP/IAR)
  uint64_t msr;     // Machine state register
  uint32_t cr;      // Condition register (8 x 4-bit fields)
  uint64_t lr;      // Link register
  uint64_t ctr;     // Count register
  uint64_t xer;     // Fixed-point exception register

  // Additional special purpose registers
  uint64_t dar;     // Data address register (for exceptions)
  uint32_t dsisr;   // Data storage interrupt status register
  uint64_t srr0;    // Save/restore register 0 (return address)
  uint64_t srr1;    // Save/restore register 1 (MSR save)

  uint32_t fpscr;   // Floating-point status and control register
  uint32_t vscr;    // Vector status and control register

  // Note: Xenon does NOT have VRSAVE register like standard AltiVec

  uint64_t tb;      // Time base (64-bit counter)
  uint32_t pvr;     // Processor version register

  // Xenon-specific processor ID
  static constexpr uint32_t XENON_PVR = 0x00710700; // Xbox 360 CPU
};

// VMX128 capability information
struct VMX128Info {
  static constexpr size_t NUM_VECTOR_REGISTERS = 128;
  static constexpr size_t VECTOR_REGISTER_SIZE = 16; // 128 bits = 16 bytes
  static constexpr bool HAS_VRSAVE = false;
  static constexpr bool IS_64BIT = true;
  static constexpr const char *VARIANT_NAME = "Xbox 360 Xenon";
};

} // namespace Xenon
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
