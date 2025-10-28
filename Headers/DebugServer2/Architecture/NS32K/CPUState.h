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
namespace NS32K {

//
// National Semiconductor 32000 series (NS32K) CPU State
// 32-bit CISC architecture (1982-1990s)
// Used in: PC532, Encore Multimax, ICL DRS, Sequent Balance
// Variants: NS32016, NS32032, NS32332, NS32532
//

struct CPUState {
  //
  // General Purpose Registers (8 x 32-bit)
  // r0-r7: General purpose, can be used for any operation
  // Register usage conventions:
  // r0-r2: Typically used for return values and temporary values
  // r3-r7: General purpose / preserved across calls (varies by ABI)
  //
  struct {
    union {
      uint32_t regs[8];
      struct {
        uint32_t r0;
        uint32_t r1;
        uint32_t r2;
        uint32_t r3;
        uint32_t r4;
        uint32_t r5;
        uint32_t r6;
        uint32_t r7;
      };
    };
  } gp;

  //
  // Dedicated Registers
  // These are separate from the general-purpose register file
  //
  struct {
    uint32_t fp;      // Frame Pointer
    uint32_t sp;      // Stack Pointer (multiple stack pointers in some modes)
    uint32_t sb;      // Static Base (for position-independent code)
    uint32_t pc;      // Program Counter
    uint32_t psr;     // Processor Status Register
    uint32_t mod;     // Module Register (MMU control)
    uint32_t intbase; // Interrupt Vector Base
    uint32_t cfg;     // Configuration Register
  } special;

  //
  // Processor Status Register (PSR) bit fields
  // Contains flags, interrupt level, and mode information
  //
  struct {
    uint32_t c : 1;      // Carry flag
    uint32_t t : 1;      // Trace trap enable
    uint32_t l : 1;      // Less than flag (signed)
    uint32_t v : 1;      // Overflow flag
    uint32_t f : 1;      // Flag bit (general purpose)
    uint32_t z : 1;      // Zero flag
    uint32_t n : 1;      // Negative flag
    uint32_t i : 1;      // Interrupt enable
    uint32_t p : 1;      // Previous mode (user/supervisor)
    uint32_t s : 1;      // Supervisor mode
    uint32_t u : 1;      // User mode
    uint32_t reserved : 21;
  } psr_flags;

  //
  // Floating-Point Unit (NS32081/NS32181/NS32381)
  // 8 floating-point registers
  // Can be accessed as:
  // - 8 single-precision (32-bit) registers: f0-f7
  // - 4 double-precision (64-bit) registers: f0, f2, f4, f6
  //
  union {
    float f32[8];       // Single-precision access
    double f64[4];      // Double-precision access (f0, f2, f4, f6)
    uint32_t raw32[8];  // Raw 32-bit access
    uint64_t raw64[4];  // Raw 64-bit access
  } fpu;

  //
  // FPU Status Register (FSR)
  //
  struct {
    uint32_t rm : 2;     // Rounding mode (00=nearest, 01=zero, 10=+inf, 11=-inf)
    uint32_t reserved1 : 3;
    uint32_t uf : 1;     // Underflow trap enable
    uint32_t if_ : 1;    // Inexact trap enable
    uint32_t tt : 1;     // Trace trap
    uint32_t reserved2 : 24;
  } fsr;

  //
  // Memory Management Unit Registers
  // NS32532 and later have on-chip MMU
  //
  struct {
    uint32_t ptb0;       // Page Table Base 0 (user mode)
    uint32_t ptb1;       // Page Table Base 1 (supervisor mode)
    uint32_t eia;        // Error/Invalidate Address
    uint32_t mcr;        // MMU Control Register
    uint32_t msr;        // MMU Status Register
    uint32_t tear;       // Translation Exception Address Register
  } mmu;

  //
  // Breakpoint and Debug Registers
  // NS32532 debug support
  //
  struct {
    uint32_t bpc;        // Breakpoint Program Counter
    uint32_t dcr;        // Debug Control Register
    uint32_t dsr;        // Debug Status Register
    uint32_t car;        // Compare Address Register
  } debug;

  //
  // Cache Control (NS32532)
  //
  struct {
    uint32_t iccr;       // Instruction Cache Control Register
    uint32_t dccr;       // Data Cache Control Register
  } cache;

  //
  // Interrupt Control
  // Multiple stack pointers for different interrupt levels
  //
  struct {
    uint32_t sp0;        // Stack Pointer level 0 (user)
    uint32_t sp1;        // Stack Pointer level 1 (interrupt level 1)
  } interrupt;
};

} // namespace NS32K
} // namespace Architecture
} // namespace ds2
