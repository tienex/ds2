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
namespace M68K {

//
// Motorola 68000 series (m68k) CPU State
// 32-bit CISC architecture (1979-present)
// Used in: Sun workstations, NeXT, Apple Macintosh, Amiga, Atari ST, VME systems
// Variants: 68000, 68010, 68020, 68030, 68040, 68060, ColdFire
//

struct CPUState {
  //
  // Data Registers (8 x 32-bit)
  // d0-d7: General purpose data registers
  // Can be accessed as byte, word, or longword
  //
  struct {
    union {
      uint32_t regs[8];
      struct {
        uint32_t d0;
        uint32_t d1;
        uint32_t d2;
        uint32_t d3;
        uint32_t d4;
        uint32_t d5;
        uint32_t d6;
        uint32_t d7;
      };
    };
  } data;

  //
  // Address Registers (7 x 32-bit + stack pointers)
  // a0-a6: General purpose address registers
  // a7 (sp): Stack pointer (either usp or ssp depending on mode)
  //
  struct {
    union {
      uint32_t regs[7];
      struct {
        uint32_t a0;
        uint32_t a1;
        uint32_t a2;
        uint32_t a3;
        uint32_t a4;
        uint32_t a5;
        uint32_t a6;
      };
    };
  } addr;

  //
  // Stack Pointers
  // The 68000 has separate user and supervisor stack pointers
  //
  struct {
    uint32_t usp;     // User Stack Pointer (a7 in user mode)
    uint32_t ssp;     // Supervisor Stack Pointer (a7 in supervisor mode)
    uint32_t msp;     // Master Stack Pointer (68020+, optional)
    uint32_t isp;     // Interrupt Stack Pointer (68020+, optional)
  } stack;

  //
  // Program Counter and Status Register
  //
  struct {
    uint32_t pc;      // Program Counter
    uint16_t sr;      // Status Register (CCR + System byte)
  } special;

  //
  // Status Register (SR) bit fields
  // Lower byte (CCR - Condition Code Register):
  //   Bits 0-4: Condition codes
  // Upper byte (System):
  //   Bits 8-15: Interrupt mask, supervisor mode, trace mode
  //
  struct {
    // Condition Code Register (CCR) - lower byte
    uint16_t c : 1;      // Carry
    uint16_t v : 1;      // Overflow
    uint16_t z : 1;      // Zero
    uint16_t n : 1;      // Negative
    uint16_t x : 1;      // Extend (for multiprecision arithmetic)
    uint16_t reserved1 : 3;
    // System byte - upper byte
    uint16_t ipm : 3;    // Interrupt Priority Mask (0-7)
    uint16_t reserved2 : 2;
    uint16_t s : 1;      // Supervisor mode
    uint16_t reserved3 : 1;
    uint16_t t : 2;      // Trace mode (68020+: 00=off, 01=on change of flow, 10=any, 11=undef)
  } sr_flags;

  //
  // Floating-Point Unit (68881/68882/68040/68060 FPU)
  // 8 extended-precision (80-bit) floating-point registers
  //
  struct {
    // 80-bit extended precision registers
    struct {
      uint64_t mantissa;   // 64-bit mantissa
      uint16_t exponent;   // 15-bit exponent + sign bit
    } fpr[8];

    // FPU Control Registers
    uint32_t fpcr;         // Floating-Point Control Register
    uint32_t fpsr;         // Floating-Point Status Register
    uint32_t fpiar;        // Floating-Point Instruction Address Register
  } fpu;

  //
  // Memory Management Unit (68851, 68030, 68040, 68060)
  //
  struct {
    // 68851/68030 MMU registers
    uint32_t crp;          // CPU Root Pointer
    uint32_t srp;          // Supervisor Root Pointer
    uint16_t tc;           // Translation Control
    uint16_t tt0;          // Transparent Translation register 0
    uint16_t tt1;          // Transparent Translation register 1
    uint32_t mmusr;        // MMU Status Register

    // 68040/68060 MMU registers
    uint32_t urp;          // User Root Pointer (68040+)
    uint32_t srp_040;      // Supervisor Root Pointer (68040+)
    uint32_t tc_040;       // Translation Control (68040+)
    uint32_t itt0;         // Instruction Transparent Translation 0 (68040+)
    uint32_t itt1;         // Instruction Transparent Translation 1 (68040+)
    uint32_t dtt0;         // Data Transparent Translation 0 (68040+)
    uint32_t dtt1;         // Data Transparent Translation 1 (68040+)
  } mmu;

  //
  // Cache Control (68020+)
  //
  struct {
    uint16_t cacr;         // Cache Control Register
    uint32_t caar;         // Cache Address Register (68020/68030)
    uint32_t acr0;         // Access Control Register 0 (ColdFire)
    uint32_t acr1;         // Access Control Register 1 (ColdFire)
  } cache;

  //
  // Special Control Registers (68010+)
  //
  struct {
    uint32_t vbr;          // Vector Base Register (68010+)
    uint16_t sfc;          // Source Function Code (68010+)
    uint16_t dfc;          // Destination Function Code (68010+)
    uint32_t cacr_ext;     // Extended Cache Control (68040+)
    uint32_t acr[4];       // Access Control Registers (ColdFire)
    uint32_t rambar;       // RAM Base Address Register (ColdFire)
    uint32_t mbar;         // Module Base Address Register (ColdFire)
  } control;

  //
  // Debug Support (68040+, ColdFire)
  //
  struct {
    uint32_t baddr[4];     // Breakpoint Address Registers
    uint32_t bctrl[4];     // Breakpoint Control Registers
    uint32_t aatr;         // Address Attribute Register
    uint32_t tdr;          // Trigger Definition Register
  } debug;
};

} // namespace M68K
} // namespace Architecture
} // namespace ds2
