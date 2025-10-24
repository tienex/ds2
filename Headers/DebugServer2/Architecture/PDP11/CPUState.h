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
namespace PDP11 {

// CPU State for DEC PDP-11
// Classic 16-bit minicomputer (1970-1990s)
// Used in Unix Version 6, Version 7, 2.11BSD, Ultrix-11

struct CPUState {
  // General purpose registers (16-bit)
  union {
    struct {
      uint16_t r0;  // Register 0
      uint16_t r1;  // Register 1
      uint16_t r2;  // Register 2
      uint16_t r3;  // Register 3
      uint16_t r4;  // Register 4
      uint16_t r5;  // Register 5 (also used as frame pointer)
      uint16_t r6;  // Stack pointer (SP)
      uint16_t r7;  // Program counter (PC)
    };
    uint16_t r[8];
  };

  // Processor Status Word (PSW)
  uint16_t psw;

  // Memory Management Unit registers (for PDP-11/45, 11/70 with MMU)
  struct {
    // Kernel mode MMU registers
    uint16_t kisar[8];  // Kernel I-space Address Registers
    uint16_t kisdr[8];  // Kernel I-space Descriptor Registers
    uint16_t kdsar[8];  // Kernel D-space Address Registers
    uint16_t kdsdr[8];  // Kernel D-space Descriptor Registers

    // Supervisor mode MMU registers (11/45, 11/70)
    uint16_t sisar[8];
    uint16_t sisdr[8];
    uint16_t sdsar[8];
    uint16_t sdsdr[8];

    // User mode MMU registers
    uint16_t uisar[8];
    uint16_t uisdr[8];
    uint16_t udsar[8];
    uint16_t udsdr[8];

    // MMU control registers
    uint16_t mmr0;      // MMU Status Register 0
    uint16_t mmr1;      // MMU Status Register 1
    uint16_t mmr2;      // MMU Status Register 2
    uint16_t mmr3;      // MMU Status Register 3 (11/70 only)
  } mmu;

  // Floating Point Unit registers (FP11, FPP)
  struct {
    double fac;         // Floating Accumulator
    uint16_t fps;       // Floating Point Status
  } fpu;

  CPUState() { memset(this, 0, sizeof(*this)); }

  // PSW (Processor Status Word) bits
  enum PSWBits {
    kPSW_C = (1 << 0),   // Carry
    kPSW_V = (1 << 1),   // Overflow
    kPSW_Z = (1 << 2),   // Zero
    kPSW_N = (1 << 3),   // Negative
    kPSW_T = (1 << 4),   // Trace trap
    kPSW_PRIORITY = (7 << 5), // Priority level (3 bits)
    // Bits 8-13: Reserved or used for extended features
    kPSW_PREVMODE = (3 << 12), // Previous mode (2 bits)
    kPSW_CURMODE = (3 << 14),  // Current mode (2 bits)
  };

  // CPU modes
  enum CPUMode {
    kModeKernel = 0,
    kModeSupervisor = 1,
    kModeReserved = 2,
    kModeUser = 3,
  };

  // Get PC (program counter)
  inline uint16_t pc() const {
    return r7;
  }

  // Set PC
  inline void setPC(uint16_t addr) {
    r7 = addr;
  }

  // Get SP (stack pointer)
  inline uint16_t sp() const {
    return r6;
  }

  // Set SP
  inline void setSP(uint16_t addr) {
    r6 = addr;
  }

  // Single step support
  inline void setSingleStep(bool enable) {
    if (enable) {
      psw |= kPSW_T;
    } else {
      psw &= ~kPSW_T;
    }
  }

  inline bool isSingleStep() const {
    return (psw & kPSW_T) != 0;
  }

  // Get current mode
  inline CPUMode getCurrentMode() const {
    return static_cast<CPUMode>((psw & kPSW_CURMODE) >> 14);
  }

  // Check if MMU is enabled (simplified)
  inline bool isMMUEnabled() const {
    return (mmu.mmr0 & 0x0001) != 0;
  }
};

// Register numbering for GDB protocol
enum GDBRegisterIndex {
  kGDBRegisterR0 = 0,
  kGDBRegisterR1 = 1,
  kGDBRegisterR2 = 2,
  kGDBRegisterR3 = 3,
  kGDBRegisterR4 = 4,
  kGDBRegisterR5 = 5,
  kGDBRegisterSP = 6,
  kGDBRegisterPC = 7,
  kGDBRegisterPSW = 8,
  kGDBRegisterCount = 9
};

} // namespace PDP11
} // namespace Architecture
} // namespace ds2
