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
namespace WE32K {

//
// Western Electric 32000 (WE32K) CPU State
// 32-bit CISC architecture by AT&T/Western Electric (1984-1990s)
// Used in: AT&T 3B2, 3B5, 3B15, 3B20 computers running UNIX System V
// Variants: WE32100, WE32200, WE32106 (with MAC unit)
//

struct CPUState {
  //
  // General Purpose Registers (9 x 32-bit)
  // r0-r8: General purpose registers
  // Register usage conventions (AT&T System V ABI):
  // r0-r2: Temporary registers (caller-saved)
  // r3-r8: Saved registers (callee-saved)
  //
  struct {
    union {
      uint32_t regs[9];
      struct {
        uint32_t r0;
        uint32_t r1;
        uint32_t r2;
        uint32_t r3;
        uint32_t r4;
        uint32_t r5;
        uint32_t r6;
        uint32_t r7;
        uint32_t r8;
      };
    };
  } gp;

  //
  // Special Purpose Registers
  //
  struct {
    uint32_t fp;      // Frame Pointer (R9)
    uint32_t ap;      // Argument Pointer (R10)
    uint32_t psw;     // Processor Status Word (R11)
    uint32_t sp;      // Stack Pointer (R12)
    uint32_t pcbp;    // Process Control Block Pointer (R13)
    uint32_t isp;     // Interrupt Stack Pointer (R14)
    uint32_t pc;      // Program Counter (R15)
  } special;

  //
  // Processor Status Word (PSW) bit fields
  // Contains condition codes, interrupt level, and processor mode
  //
  struct {
    uint32_t c : 1;      // Carry flag
    uint32_t v : 1;      // Overflow flag
    uint32_t z : 1;      // Zero flag
    uint32_t n : 1;      // Negative flag
    uint32_t oe : 1;     // Overflow Exception enable
    uint32_t cd : 1;     // Cache Disable
    uint32_t qie : 1;    // Quick Interrupt Enable
    uint32_t cfd : 1;    // Cache Flush Disable
    uint32_t reserved1 : 3;
    uint32_t isc : 4;    // Interrupt State Code
    uint32_t tm : 1;     // Trace Mode
    uint32_t reserved2 : 1;
    uint32_t cm : 2;     // Current execution Mode (00=kernel, 01=exec, 10=super, 11=user)
    uint32_t ipl : 4;    // Interrupt Priority Level (0-15)
    uint32_t r : 1;      // Register set (0=normal, 1=alternate)
    uint32_t pm : 2;     // Previous execution Mode
    uint32_t reserved3 : 8;
  } psw_flags;

  //
  // Math Accelerator Unit (MAU) - Optional FPU
  // Available on WE32106 and some WE32200 systems
  // Provides hardware floating-point support
  //
  struct {
    // 4 double-precision (64-bit) registers
    // Can be accessed as 4 double or 8 single-precision
    union {
      double d[4];       // Double-precision (64-bit): d0, d1, d2, d3
      float s[8];        // Single-precision (32-bit): s0-s7
      uint64_t raw64[4]; // Raw 64-bit access
      uint32_t raw32[8]; // Raw 32-bit access
    } regs;

    // MAU Status Register (MASR)
    struct {
      uint32_t ie : 1;     // Inexact result
      uint32_t ue : 1;     // Underflow
      uint32_t oe : 1;     // Overflow
      uint32_t de : 1;     // Divide by zero
      uint32_t ie_mask : 1; // Inexact exception mask
      uint32_t ue_mask : 1; // Underflow exception mask
      uint32_t oe_mask : 1; // Overflow exception mask
      uint32_t de_mask : 1; // Divide by zero exception mask
      uint32_t rm : 2;     // Rounding mode (00=nearest, 01=zero, 10=+inf, 11=-inf)
      uint32_t reserved : 22;
    } masr;

    // MAU Control Register (MACR)
    uint32_t macr;
  } mau;

  //
  // Memory Management Unit (MMU) Registers
  //
  struct {
    uint32_t srama[64];   // Segment Descriptor RAM A (64 entries)
    uint32_t sramb[64];   // Segment Descriptor RAM B (64 entries)
    uint32_t fltcr;       // Fault Control Register
    uint32_t fltadr;      // Fault Address Register
    uint32_t sdr[3];      // Segment Descriptor Registers (SD0-SD2)
  } mmu;

  //
  // System Control Registers
  //
  struct {
    uint32_t ivtp;        // Interrupt Vector Table Pointer
    uint32_t acr;         // Abort Control Register
    uint32_t asr;         // Abort Status Register
    uint32_t uar;         // Unaligned Access Register
    uint32_t tcr;         // Timer Control Register
  } system;

  //
  // Debug and Breakpoint Support
  //
  struct {
    uint32_t dr0;         // Debug Register 0
    uint32_t dr1;         // Debug Register 1
    uint32_t dr2;         // Debug Register 2
    uint32_t dr3;         // Debug Register 3
    uint32_t dcr;         // Debug Control Register
    uint32_t dsr;         // Debug Status Register
  } debug;
};

} // namespace WE32K
} // namespace Architecture
} // namespace ds2
