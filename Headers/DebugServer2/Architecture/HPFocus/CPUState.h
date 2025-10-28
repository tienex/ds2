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
namespace HPFocus {

// CPU State for HP Focus
// Precursor to HP Precision Architecture (PA-RISC)
// 32-bit RISC architecture used in early HP 9000 systems

struct CPUState {
  // General purpose registers (32-bit)
  union {
    struct {
      uint32_t r0;   // Always zero (like MIPS)
      uint32_t r1;   // Return pointer
      uint32_t r2;   // Stack pointer (SP)
      uint32_t r3;   // Frame pointer (FP)
      uint32_t r4;   // Thread pointer
      uint32_t r5;   // Preserved register
      uint32_t r6;   // Preserved register
      uint32_t r7;   // Preserved register
      uint32_t r8;   // Preserved register
      uint32_t r9;   // Preserved register
      uint32_t r10;  // Preserved register
      uint32_t r11;  // Preserved register
      uint32_t r12;  // Preserved register
      uint32_t r13;  // Preserved register
      uint32_t r14;  // Preserved register
      uint32_t r15;  // Preserved register
      uint32_t r16;  // Argument 0
      uint32_t r17;  // Argument 1
      uint32_t r18;  // Argument 2
      uint32_t r19;  // Argument 3
      uint32_t r20;  // Temporary
      uint32_t r21;  // Temporary
      uint32_t r22;  // Temporary
      uint32_t r23;  // Temporary
      uint32_t r24;  // Temporary
      uint32_t r25;  // Temporary
      uint32_t r26;  // Return value 0
      uint32_t r27;  // Data pointer
      uint32_t r28;  // Return value 1
      uint32_t r29;  // Global pointer
      uint32_t r30;  // Scratch
      uint32_t r31;  // Millicode return
    };
    uint32_t r[32];
  };

  // Control registers
  uint32_t pc;          // Program Counter
  uint32_t npc;         // Next Program Counter (delayed branch)

  // Processor Status Word
  uint32_t psw;

  // Space registers (for segmentation)
  uint32_t sr[8];

  // Control Registers
  struct {
    uint32_t rctr;      // Recovery Counter
    uint32_t cr_pidr1;  // Protection ID 1
    uint32_t cr_pidr2;  // Protection ID 2
    uint32_t cr_ccr;    // Coprocessor Configuration Register
    uint32_t cr_sar;    // Shift Amount Register
    uint32_t cr_pidr3;  // Protection ID 3
    uint32_t cr_pidr4;  // Protection ID 4
    uint32_t cr_iva;    // Interrupt Vector Address
    uint32_t cr_eiem;   // External Interrupt Enable Mask
    uint32_t cr_itmr;   // Interval Timer
    uint32_t cr_pcsq;   // Program Counter Space Queue
    uint32_t cr_pcoq;   // Program Counter Offset Queue
    uint32_t cr_iir;    // Interruption Instruction Register
    uint32_t cr_isr;    // Interruption Space Register
    uint32_t cr_ior;    // Interruption Offset Register
    uint32_t cr_ipsw;   // Interruption Processor Status Word
    uint32_t cr_eirr;   // External Interrupt Request Register
  } cr;

  // Floating Point registers
  struct {
    double fpr[32];     // FP registers
    uint32_t fpsr;      // FP Status Register
  } fpu;

  CPUState() { memset(this, 0, sizeof(*this)); }

  // PSW (Processor Status Word) bits
  enum PSWBits {
    kPSW_Y = (1 << 0),   // Data Debug Trap
    kPSW_Z = (1 << 1),   // Instruction Debug Trap
    kPSW_r2 = (1 << 2),  // Reserved
    kPSW_r3 = (1 << 3),  // Reserved
    kPSW_r4 = (1 << 4),  // Reserved
    kPSW_E = (1 << 5),   // Little Endian
    kPSW_S = (1 << 6),   // Secure
    kPSW_T = (1 << 7),   // Taken Branch Trap
    kPSW_H = (1 << 8),   // Higher Privilege Transfer Trap
    kPSW_L = (1 << 9),   // Lower Privilege Transfer Trap
    kPSW_N = (1 << 10),  // Nullify
    kPSW_X = (1 << 11),  // Data Translation Enable
    kPSW_B = (1 << 12),  // Branch Taken
    kPSW_C = (1 << 13),  // Code Translation Enable
    kPSW_V = (1 << 14),  // Divide Overflow Trap
    kPSW_M = (1 << 15),  // High-Priority Machine Check Mask
    kPSW_CB = (3 << 16), // Carry/Borrow bits (2 bits)
    kPSW_r18 = (1 << 18), // Reserved
    kPSW_r19 = (1 << 19), // Reserved
    kPSW_r20 = (1 << 20), // Reserved
    kPSW_r21 = (1 << 21), // Reserved
    kPSW_r22 = (1 << 22), // Reserved
    kPSW_I = (1 << 23),  // External Interrupt Enable
    kPSW_Q = (1 << 24),  // Interrupt State Collection Enable
    kPSW_P = (1 << 25),  // Protection Enable
    kPSW_D = (1 << 26),  // Data Translation Enable
    kPSW_R = (1 << 27),  // Recover Counter Enable
  };

  // Get PC
  inline uint32_t getPC() const {
    return pc;
  }

  // Set PC
  inline void setPC(uint32_t addr) {
    pc = addr;
    npc = addr + 4; // Simple case, not handling delayed branches
  }

  // Get SP
  inline uint32_t getSP() const {
    return r2;
  }

  // Get return address
  inline uint32_t retaddr() const {
    return r1;
  }

  // Single step support
  inline void setSingleStep(bool enable) {
    // Use debug trap bits
    if (enable) {
      psw |= (kPSW_Y | kPSW_Z);
    } else {
      psw &= ~(kPSW_Y | kPSW_Z);
    }
  }

  inline bool isSingleStep() const {
    return ((psw & kPSW_Y) != 0) || ((psw & kPSW_Z) != 0);
  }
};

// Register numbering for GDB protocol
enum GDBRegisterIndex {
  kGDBRegisterR0 = 0,
  // ... R1-R30
  kGDBRegisterR31 = 31,
  kGDBRegisterPC = 32,
  kGDBRegisterPSW = 33,
  kGDBRegisterCount = 34
};

} // namespace HPFocus
} // namespace Architecture
} // namespace ds2
