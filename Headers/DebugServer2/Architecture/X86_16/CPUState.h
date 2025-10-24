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
namespace X86_16 {

// CPU State for 16-bit x86 (8086, 80186, 80286)
// Supports both real mode and protected mode (80286)

struct CPUState {
  // General purpose registers (16-bit)
  union {
    struct {
      uint16_t ax;  // Accumulator
      uint16_t cx;  // Counter
      uint16_t dx;  // Data
      uint16_t bx;  // Base
      uint16_t sp;  // Stack pointer
      uint16_t bp;  // Base pointer
      uint16_t si;  // Source index
      uint16_t di;  // Destination index
    };
    uint16_t gp[8];
  };

  // Segment registers (16-bit)
  union {
    struct {
      uint16_t es;  // Extra segment
      uint16_t cs;  // Code segment
      uint16_t ss;  // Stack segment
      uint16_t ds;  // Data segment
    };
    uint16_t seg[4];
  };

  // Instruction pointer and flags (16-bit)
  uint16_t ip;      // Instruction pointer
  uint16_t flags;   // FLAGS register

  // 80286 Protected mode registers (optional)
  struct {
    uint16_t gdtr_limit;   // Global Descriptor Table Limit
    uint32_t gdtr_base;    // Global Descriptor Table Base (24-bit on 286)
    uint16_t idtr_limit;   // Interrupt Descriptor Table Limit
    uint32_t idtr_base;    // Interrupt Descriptor Table Base (24-bit on 286)
    uint16_t ldtr;         // Local Descriptor Table Register
    uint16_t tr;           // Task Register
    uint16_t msw;          // Machine Status Word
  } protected_mode;

  // Debug registers (80386 and later, not present on 8086/286)
  // Included for completeness but typically zero on 8086/286
  uint32_t dr[8];

  CPUState() { memset(this, 0, sizeof(*this)); }

  // FLAGS register bits
  enum FlagBits {
    kFlagCF = (1 << 0),   // Carry flag
    kFlagPF = (1 << 2),   // Parity flag
    kFlagAF = (1 << 4),   // Auxiliary carry flag
    kFlagZF = (1 << 6),   // Zero flag
    kFlagSF = (1 << 7),   // Sign flag
    kFlagTF = (1 << 8),   // Trap flag (single step)
    kFlagIF = (1 << 9),   // Interrupt enable flag
    kFlagDF = (1 << 10),  // Direction flag
    kFlagOF = (1 << 11),  // Overflow flag
    kFlagIOPL = (3 << 12),// I/O privilege level (286+ protected mode)
    kFlagNT = (1 << 14),  // Nested task flag (286+ protected mode)
  };

  // Machine Status Word bits (80286 protected mode)
  enum MSWBits {
    kMSW_PE = (1 << 0),   // Protection Enable
    kMSW_MP = (1 << 1),   // Monitor Processor Extension
    kMSW_EM = (1 << 2),   // Emulate Processor Extension
    kMSW_TS = (1 << 3),   // Task Switched
  };

  // Get linear address from segment:offset
  static inline uint32_t linearAddress(uint16_t segment, uint16_t offset) {
    return (static_cast<uint32_t>(segment) << 4) + offset;
  }

  // Get PC (program counter)
  inline uint32_t pc() const {
    return linearAddress(cs, ip);
  }

  // Set PC (program counter)
  inline void setPC(uint32_t addr) {
    // In real mode, convert linear address to segment:offset
    // This is a simplified conversion
    cs = static_cast<uint16_t>(addr >> 4);
    ip = static_cast<uint16_t>(addr & 0xF);
  }

  // Get SP (stack pointer)
  inline uint32_t getSP() const {
    return linearAddress(ss, sp);
  }

  // Single step support
  inline void setSingleStep(bool enable) {
    if (enable) {
      flags |= kFlagTF;
    } else {
      flags &= ~kFlagTF;
    }
  }

  inline bool isSingleStep() const {
    return (flags & kFlagTF) != 0;
  }

  // Get return address from stack
  inline uint32_t retaddr() const {
    // In 16-bit mode, return address is at [SS:SP]
    return getSP();
  }
};

// Register numbering for GDB protocol
enum GDBRegisterIndex {
  kGDBRegisterAX = 0,
  kGDBRegisterCX = 1,
  kGDBRegisterDX = 2,
  kGDBRegisterBX = 3,
  kGDBRegisterSP = 4,
  kGDBRegisterBP = 5,
  kGDBRegisterSI = 6,
  kGDBRegisterDI = 7,
  kGDBRegisterIP = 8,
  kGDBRegisterFLAGS = 9,
  kGDBRegisterCS = 10,
  kGDBRegisterSS = 11,
  kGDBRegisterDS = 12,
  kGDBRegisterES = 13,
  kGDBRegisterCount = 14
};

} // namespace X86_16
} // namespace Architecture
} // namespace ds2
