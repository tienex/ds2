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
namespace MMIX {

// CPU State for MMIX
// Donald Knuth's 64-bit RISC architecture
// Described in "The Art of Computer Programming" and "MMIX: A RISC Computer for the New Millennium"

struct CPUState {
  // General purpose registers (64-bit)
  // 256 general registers ($0-$255)
  uint64_t g[256];

  // Special registers (64-bit)
  uint64_t rA;      // Arithmetic register
  uint64_t rB;      // Bootstrap register (trap address)
  uint64_t rC;      // Cycle counter
  uint64_t rD;      // Dividend register
  uint64_t rE;      // Epsilon register
  uint64_t rF;      // Failure location register
  uint64_t rG;      // Global threshold register
  uint64_t rH;      // Himult register
  uint64_t rI;      // Interval counter
  uint64_t rJ;      // Return-jump register
  uint64_t rK;      // Interrupt mask register
  uint64_t rL;      // Local threshold register
  uint64_t rM;      // Multiplex mask register
  uint64_t rN;      // Serial number
  uint64_t rO;      // Register stack offset
  uint64_t rP;      // Prediction register
  uint64_t rQ;      // Interrupt request register
  uint64_t rR;      // Remainder register
  uint64_t rS;      // Register stack pointer
  uint64_t rT;      // Trap address register
  uint64_t rU;      // Usage counter
  uint64_t rV;      // Virtual translation register
  uint64_t rW;      // Where-interrupted register (PC)
  uint64_t rX;      // Execution register
  uint64_t rY;      // Y operand
  uint64_t rZ;      // Z operand
  uint64_t rBB;     // Bootstrap register (trap)
  uint64_t rTT;     // Dynamic trap address register
  uint64_t rWW;     // Where interrupted register
  uint64_t rXX;     // Execution register
  uint64_t rYY;     // Y operand
  uint64_t rZZ;     // Z operand

  // Program Counter (not a special register, but needed for debugging)
  uint64_t pc;

  CPUState() { memset(this, 0, sizeof(*this)); }

  // Special register indices
  enum SpecialRegister {
    kRegA = 21,    // rA - arithmetic
    kRegB = 0,     // rB - bootstrap
    kRegC = 8,     // rC - cycle
    kRegD = 1,     // rD - dividend
    kRegE = 2,     // rE - epsilon
    kRegF = 22,    // rF - failure
    kRegG = 19,    // rG - global
    kRegH = 3,     // rH - himult
    kRegI = 12,    // rI - interval
    kRegJ = 4,     // rJ - return jump
    kRegK = 15,    // rK - interrupt mask
    kRegL = 20,    // rL - local
    kRegM = 5,     // rM - multiplex mask
    kRegN = 9,     // rN - serial number
    kRegO = 10,    // rO - register stack offset
    kRegP = 23,    // rP - prediction
    kRegQ = 16,    // rQ - interrupt request
    kRegR = 6,     // rR - remainder
    kRegS = 11,    // rS - register stack pointer
    kRegT = 13,    // rT - trap address
    kRegU = 17,    // rU - usage counter
    kRegV = 18,    // rV - virtual translation
    kRegW = 24,    // rW - where interrupted
    kRegX = 25,    // rX - execution
    kRegY = 26,    // rY - Y operand
    kRegZ = 27,    // rZ - Z operand
  };

  // Get PC
  inline uint64_t getPC() const {
    return pc;
  }

  // Set PC
  inline void setPC(uint64_t addr) {
    pc = addr;
  }

  // Get SP (conventionally $254)
  inline uint64_t getSP() const {
    return g[254];
  }

  // Set SP
  inline void setSP(uint64_t addr) {
    g[254] = addr;
  }

  // Get return address (rJ - return-jump register)
  inline uint64_t retaddr() const {
    return rJ;
  }

  // Get global threshold (number of global registers)
  inline uint8_t getGlobalThreshold() const {
    return static_cast<uint8_t>(rG & 0xFF);
  }

  // Get local threshold (number of local registers)
  inline uint8_t getLocalThreshold() const {
    return static_cast<uint8_t>(rL & 0xFF);
  }

  // Check if register is global
  inline bool isGlobalRegister(uint8_t regnum) const {
    return regnum >= (256 - getGlobalThreshold());
  }

  // Check if register is local
  inline bool isLocalRegister(uint8_t regnum) const {
    uint8_t L = getLocalThreshold();
    uint8_t G = getGlobalThreshold();
    return regnum >= L && regnum < (256 - G);
  }

  // Single step support (use rK interrupt mask)
  inline void setSingleStep(bool enable) {
    if (enable) {
      rK |= (1ULL << 0); // Enable trace interrupt
    } else {
      rK &= ~(1ULL << 0);
    }
  }

  inline bool isSingleStep() const {
    return (rK & (1ULL << 0)) != 0;
  }

  // MMIX uses register windowing similar to SPARC
  // Register $0 is always 0 (like MIPS)
  inline uint64_t getRegister(uint8_t regnum) const {
    if (regnum == 0) return 0;
    return g[regnum];
  }

  inline void setRegister(uint8_t regnum, uint64_t value) {
    if (regnum != 0) {
      g[regnum] = value;
    }
  }
};

// Register numbering for GDB protocol
enum GDBRegisterIndex {
  kGDBRegisterG0 = 0,
  // ... G1-G254
  kGDBRegisterG255 = 255,
  kGDBRegisterPC = 256,
  kGDBRegisterRA = 257,   // rA
  kGDBRegisterRJ = 258,   // rJ (return address)
  kGDBRegisterCount = 259
};

} // namespace MMIX
} // namespace Architecture
} // namespace ds2
