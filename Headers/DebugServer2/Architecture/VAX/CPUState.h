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
namespace VAX {

//
// VAX (Virtual Address eXtension) CPU State
// Covers: VAX-11/780 through VAX 7000/10000 series
// DEC's 32-bit architecture (1977-2000)
//

struct CPUState {
  //
  // General Purpose Registers (R0-R15)
  // 16 general-purpose 32-bit registers
  //
  struct {
    union {
      uint32_t regs[16];
      struct {
        uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
        uint32_t r8, r9, r10, r11;
        uint32_t ap;     // R12: Argument Pointer
        uint32_t fp;     // R13: Frame Pointer
        uint32_t sp;     // R14: Stack Pointer
        uint32_t pc;     // R15: Program Counter
      };
    };
  } gp;

  //
  // Processor Status Longword (PSL)
  // 32-bit processor status
  //
  struct {
    uint32_t c : 1;     // Carry
    uint32_t v : 1;     // Overflow
    uint32_t z : 1;     // Zero
    uint32_t n : 1;     // Negative
    uint32_t t : 1;     // Trace trap enable
    uint32_t iv : 1;    // Integer overflow trap enable
    uint32_t fu : 1;    // Floating underflow trap enable
    uint32_t dv : 1;    // Decimal overflow trap enable
    uint32_t ipl : 5;   // Interrupt Priority Level (bits 8-12)
    uint32_t _reserved1 : 3; // Reserved bits 13-15
    uint32_t prvmod : 2; // Previous mode (kernel, exec, super, user)
    uint32_t curmod : 2; // Current mode (kernel, exec, super, user)
    uint32_t is : 1;    // Interrupt Stack
    uint32_t fpd : 1;   // First Part Done
    uint32_t _reserved2 : 1; // Reserved bit 22
    uint32_t tp : 1;    // Trace Pending
    uint32_t cm : 1;    // Compatibility Mode
    uint32_t _reserved3 : 7; // Reserved bits 25-31
  } psl;

  //
  // Floating-Point Registers
  // VAX supports multiple floating-point formats
  // Not all VAX models have hardware floating-point
  //

  // F-format (32-bit floating-point) - Single precision
  struct {
    uint32_t f[16];   // F0-F15 (not all models support all 16)
  } f_float;

  // D-format (64-bit floating-point) - Double precision
  struct {
    uint64_t d[16];   // D0-D15 (not all models support all 16)
  } d_float;

  // G-format (64-bit floating-point) - Extended double precision
  struct {
    uint64_t g[16];   // G0-G15 (VAX-11/750 and later)
  } g_float;

  // H-format (128-bit floating-point) - Quad precision
  struct {
    struct {
      uint64_t low;
      uint64_t high;
    } h[16];          // H0-H15 (VAX 6000/7000/8000/10000 series)
  } h_float;

  //
  // Memory Management Registers
  //
  struct {
    // Page Table Base Registers
    uint32_t p0br;    // P0 (user) Base Register
    uint32_t p0lr;    // P0 Length Register
    uint32_t p1br;    // P1 (user stack) Base Register
    uint32_t p1lr;    // P1 Length Register
    uint32_t sbr;     // System Base Register
    uint32_t slr;     // System Length Register

    // Translation Buffer Invalidate
    uint32_t tbia;    // TB Invalidate All
    uint32_t tbis;    // TB Invalidate Single

    // Memory Management Enable
    uint32_t mapen;   // Memory management enable
  } mmu;

  //
  // System Control Registers
  //
  struct {
    uint32_t ksp;     // Kernel Stack Pointer
    uint32_t esp;     // Executive Stack Pointer
    uint32_t ssp;     // Supervisor Stack Pointer
    uint32_t usp;     // User Stack Pointer

    uint32_t isp;     // Interrupt Stack Pointer
    uint32_t scbb;    // System Control Block Base

    uint32_t pcbb;    // Process Control Block Base
    uint32_t sirr;    // Software Interrupt Request Register
    uint32_t sisr;    // Software Interrupt Summary Register

    uint32_t iccs;    // Interval Clock Control/Status
    uint32_t nicr;    // Next Interval Count Register
    uint32_t icr;     // Interval Count Register
    uint32_t todr;    // Time Of Day Register

    uint32_t rxcs;    // Console Receiver Control/Status
    uint32_t rxdb;    // Console Receiver Data Buffer
    uint32_t txcs;    // Console Transmitter Control/Status
    uint32_t txdb;    // Console Transmitter Data Buffer

    uint32_t mapen;   // Memory Management Enable
    uint32_t tbia;    // Translation Buffer Invalidate All
    uint32_t tbis;    // Translation Buffer Invalidate Single
    uint32_t pme;     // Performance Monitor Enable
  } system;

  //
  // Cache Control (VAX 8600 and later)
  //
  struct {
    uint32_t cier;    // Cache Invalidate Enable Register
    uint32_t cci;     // Clear Cache Index
    uint32_t dci;     // Diagnostic Cache Index
  } cache;

  //
  // Vector Processing (VAX 9000 series)
  //
  struct {
    uint32_t vp : 1;  // Vector Processor present
    uint32_t _reserved : 31;

    // Vector registers (if VP present)
    struct {
      uint64_t v[16][8]; // 16 vector registers, each 512 bits (8 x 64-bit elements)
    } vreg;

    uint32_t vmr;     // Vector Mask Register
    uint32_t vsr;     // Vector Status Register
    uint32_t vlr;     // Vector Length Register
    uint32_t vcr;     // Vector Control Register
  } vector;

  //
  // Performance Monitoring (VAX 6000/7000/8000/10000)
  //
  struct {
    uint32_t pmcr[16]; // Performance Monitor Control Registers
    uint32_t pmdr[16]; // Performance Monitor Data Registers
  } perfmon;

  //
  // Internal Processor Registers (IPR)
  // Accessible via MFPR/MTPR instructions
  //
  struct {
    uint32_t asten;   // AST Enable
    uint32_t astlvl;  // AST Level
    uint32_t cmkrnl;  // Change Mode to Kernel
    uint32_t cmexec;  // Change Mode to Executive
    uint32_t cmsupr;  // Change Mode to Supervisor
    uint32_t cmuser;  // Change Mode to User

    uint32_t sysptbr; // System Page Table Base Register
    uint32_t pctag;   // Primary Cache Tag
    uint32_t pcidx;   // Primary Cache Index
    uint32_t scbb_dup;// SCBB duplicate
    uint32_t pcbb_dup;// PCBB duplicate
  } ipr;

  //
  // Exception/Interrupt State
  //
  struct {
    uint32_t scb_offset; // System Control Block offset
    uint32_t exception_pc; // PC at exception
    uint32_t exception_psl; // PSL at exception
    uint32_t fault_va;    // Faulting virtual address
    uint32_t fault_info;  // Fault information
  } exception;

  //
  // Compatibility Mode (PDP-11 emulation)
  // Available on some VAX models
  //
  struct {
    uint32_t enabled : 1;
    uint32_t _reserved : 31;

    // PDP-11 register set when in compat mode
    struct {
      uint16_t r[8];  // R0-R7 (R6=SP, R7=PC in PDP-11)
      uint16_t psw;   // PDP-11 Processor Status Word
    } pdp11;
  } compat;

  //
  // Debug/Trace Support
  //
  struct {
    uint32_t trace_enable;
    uint32_t breakpoint[4];  // Hardware breakpoints (on some models)
    uint32_t watch[4];       // Watchpoints (on some models)
  } debug;

  //
  // Model-specific features
  //
  struct {
    uint32_t model_id;       // VAX model identifier
    uint32_t cpu_revision;   // CPU revision
    uint32_t features;       // Feature flags
    uint32_t serial_number;  // CPU serial number
  } cpuinfo;
};

} // namespace VAX
} // namespace Architecture
} // namespace ds2
