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
namespace M88K {

//
// Motorola 88000 (m88k) CPU State
// RISC architecture used in late 1980s - mid 1990s
// Used in systems from Data General, Motorola, and others
//

struct CPUState {
  //
  // General Purpose Registers (32 x 32-bit)
  // r0: Always reads as zero (hardwired)
  // r1: Return address / subroutine register
  // r2-r9: Argument and return value registers
  // r10-r13: Temporary registers (caller-saved)
  // r14-r25: Saved registers (callee-saved)
  // r26-r29: Reserved for system use
  // r30: Frame pointer
  // r31: Stack pointer
  //
  struct {
    union {
      uint32_t regs[32];
      struct {
        uint32_t r0;    // Hardwired to zero
        uint32_t r1;    // Return address / subroutine register
        uint32_t r2;    // Argument/return value
        uint32_t r3;    // Argument/return value
        uint32_t r4;    // Argument register
        uint32_t r5;    // Argument register
        uint32_t r6;    // Argument register
        uint32_t r7;    // Argument register
        uint32_t r8;    // Argument register
        uint32_t r9;    // Argument register
        uint32_t r10;   // Temporary (caller-saved)
        uint32_t r11;   // Temporary (caller-saved)
        uint32_t r12;   // Temporary (caller-saved)
        uint32_t r13;   // Temporary (caller-saved)
        uint32_t r14;   // Saved register (callee-saved)
        uint32_t r15;   // Saved register (callee-saved)
        uint32_t r16;   // Saved register (callee-saved)
        uint32_t r17;   // Saved register (callee-saved)
        uint32_t r18;   // Saved register (callee-saved)
        uint32_t r19;   // Saved register (callee-saved)
        uint32_t r20;   // Saved register (callee-saved)
        uint32_t r21;   // Saved register (callee-saved)
        uint32_t r22;   // Saved register (callee-saved)
        uint32_t r23;   // Saved register (callee-saved)
        uint32_t r24;   // Saved register (callee-saved)
        uint32_t r25;   // Saved register (callee-saved)
        uint32_t r26;   // Reserved for system
        uint32_t r27;   // Reserved for system
        uint32_t r28;   // Reserved for system
        uint32_t r29;   // Reserved for system
        uint32_t r30;   // Frame pointer
        uint32_t r31;   // Stack pointer
      };
    };
  } gp;

  //
  // Program Counter and Special Registers
  //
  struct {
    uint32_t pc;      // Program Counter
    uint32_t npc;     // Next Program Counter (delayed branch)
    uint32_t fpecr;   // Floating-Point Exception Cause Register
    uint32_t fpcr;    // Floating-Point Control Register
    uint32_t fpsr;    // Floating-Point Status Register
  } special;

  //
  // Processor Status Register (PSR)
  // Contains processor mode, interrupt mask, condition codes, etc.
  //
  struct {
    uint32_t mode : 1;      // Mode: 0=supervisor, 1=user
    uint32_t rbo : 1;       // Register Bank Organization
    uint32_t ser : 1;       // Serial mode
    uint32_t carry : 1;     // Carry flag
    uint32_t reserved1 : 4;
    uint32_t sfu1d : 1;     // SFU1 disable
    uint32_t mxm : 1;       // Misaligned exception mode
    uint32_t reserved2 : 3;
    uint32_t shadow : 1;    // Shadow registers mode
    uint32_t reserved3 : 1;
    uint32_t c : 1;         // Carry
    uint32_t reserved4 : 16;
  } psr;

  //
  // Floating-Point Registers
  // 32 registers that can be accessed as:
  // - 32 single-precision (32-bit) registers: x0-x31
  // - 16 double-precision (64-bit) registers: x0-x30 (even only)
  // MC88110 extended FPU with graphics support
  //
  union {
    float s[32];        // Single-precision access (32-bit)
    double d[16];       // Double-precision access (64-bit)
    uint32_t raw32[32]; // Raw 32-bit access
    uint64_t raw64[16]; // Raw 64-bit access
  } fpu;

  //
  // Control Registers (CR)
  // Supervisor-mode control registers
  //
  struct {
    uint32_t cr0;     // PID (Process ID)
    uint32_t cr1;     // PSR (Processor Status Register)
    uint32_t cr2;     // EPSR (Exception PSR)
    uint32_t cr3;     // SSBR (Shadow Scoreboard Register)
    uint32_t cr4;     // SXIP (Shadow Execute IP)
    uint32_t cr5;     // SNIP (Shadow Next IP)
    uint32_t cr6;     // SFIP (Shadow Fetch IP)
    uint32_t cr7;     // VBR (Vector Base Register)
    uint32_t cr8;     // DMT0 (Data Memory Transaction 0)
    uint32_t cr9;     // DMD0 (Data Memory Data 0)
    uint32_t cr10;    // DMA0 (Data Memory Address 0)
    uint32_t cr11;    // DMT1 (Data Memory Transaction 1)
    uint32_t cr12;    // DMD1 (Data Memory Data 1)
    uint32_t cr13;    // DMA1 (Data Memory Address 1)
    uint32_t cr14;    // DMT2 (Data Memory Transaction 2)
    uint32_t cr15;    // DMD2 (Data Memory Data 2)
    uint32_t cr16;    // DMA2 (Data Memory Address 2)
    uint32_t cr17;    // SR0 (Supervisor Register 0)
    uint32_t cr18;    // SR1 (Supervisor Register 1)
    uint32_t cr19;    // SR2 (Supervisor Register 2)
    uint32_t cr20;    // SR3 (Supervisor Register 3)
  } cr;

  //
  // MC88110 Specific Features
  // Extended registers for MC88110 (second-generation)
  //
  struct {
    // Graphics Unit Registers (MC88110)
    uint32_t graphics_mode;
    uint32_t pixel_command;

    // Performance Monitoring (MC88110)
    struct {
      uint32_t event_counter[4];
      uint32_t event_select[4];
    } perf;

    // Extended Control
    uint32_t icmd;      // Instruction Cache Command
    uint32_t ictl;      // Instruction Cache Control
    uint32_t dcmd;      // Data Cache Command
    uint32_t dctl;      // Data Cache Control
  } mc88110;

  //
  // MMU Registers
  // Memory Management Unit control
  //
  struct {
    uint32_t batc[8];   // Block Address Translation Cache (BATC)
    uint32_t patc[32];  // Page Address Translation Cache (PATC)
    uint32_t ictl;      // Instruction Cache Control
    uint32_t dctl;      // Data Cache Control
  } mmu;
};

} // namespace M88K
} // namespace Architecture
} // namespace ds2
