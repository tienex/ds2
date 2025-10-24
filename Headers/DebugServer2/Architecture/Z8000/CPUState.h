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
namespace Z8000 {

// CPU State for Zilog Z8000/Z8001/Z8002
// Z8001: Segmented (23-bit address space)
// Z8002: Non-segmented (16-bit address space)
// Z8003/Z8004: Later variants with enhancements

struct CPUState {
  // General purpose registers (16-bit)
  // Can be used as 16-bit (R0-R15) or 32-bit pairs (RR0-RR14, even numbers only)
  union {
    uint16_t r[16];      // R0-R15 (16-bit access)
    uint32_t rr[8];      // RR0, RR2, RR4, RR6, RR8, RR10, RR12, RR14 (32-bit pairs)
  };

  // Program Counter
  union {
    uint32_t pc;         // Full PC (23-bit on Z8001, 16-bit on Z8002)
    struct {
      uint16_t pc_offset;    // PC offset
      uint8_t pc_segment;    // PC segment (Z8001 only, 7-bit)
      uint8_t _pc_reserved;
    };
  };

  // Flags and Control Word (FCW)
  uint16_t fcw;

  // System stack pointer (for segmented mode)
  union {
    uint32_t ssp;        // System stack pointer
    struct {
      uint16_t ssp_offset;
      uint8_t ssp_segment;
      uint8_t _ssp_reserved;
    };
  };

  // Normal stack pointer (for segmented mode)
  union {
    uint32_t nsp;        // Normal stack pointer
    struct {
      uint16_t nsp_offset;
      uint8_t nsp_segment;
      uint8_t _nsp_reserved;
    };
  };

  // Program Status Area Pointer (PSAP) - for context switching
  uint16_t psap;

  // Refresh counter (REFRESH)
  uint16_t refresh;

  // New System Stack Pointer (NSSP) - Z8001 segmented mode
  union {
    uint32_t nssp;
    struct {
      uint16_t nssp_offset;
      uint8_t nssp_segment;
      uint8_t _nssp_reserved;
    };
  };

  CPUState() { memset(this, 0, sizeof(*this)); }

  // Flags and Control Word (FCW) bits
  enum FCWBits {
    kFCW_C = (1 << 0),    // Carry flag
    kFCW_Z = (1 << 1),    // Zero flag
    kFCW_S = (1 << 2),    // Sign flag
    kFCW_P = (1 << 3),    // Parity/Overflow flag
    kFCW_V = (1 << 4),    // Overflow flag
    kFCW_H = (1 << 5),    // Half-carry flag
    // Bits 6-7: Reserved
    kFCW_SEG = (1 << 8),  // Segmented mode (1=segmented Z8001, 0=non-seg Z8002)
    kFCW_EPA = (1 << 9),  // Extended Processor Architecture
    kFCW_NVIE = (1 << 10),// Non-vectored interrupt enable
    kFCW_VIE = (1 << 11), // Vectored interrupt enable
    // Bits 12-15: System/Normal mode and other control bits
  };

  // Special register indices
  enum SpecialRegisters {
    kRegR0 = 0,    // Can be used for various purposes
    kRegR14 = 14,  // Often used as frame pointer
    kRegR15 = 15,  // Stack pointer in non-segmented mode
  };

  // Get PC for non-segmented mode
  inline uint32_t getPC() const {
    if (fcw & kFCW_SEG) {
      // Segmented mode: combine segment and offset
      return (static_cast<uint32_t>(pc_segment) << 16) | pc_offset;
    } else {
      // Non-segmented mode: just the offset
      return pc_offset;
    }
  }

  // Set PC
  inline void setPC(uint32_t addr) {
    if (fcw & kFCW_SEG) {
      // Segmented mode
      pc_segment = static_cast<uint8_t>((addr >> 16) & 0x7F);
      pc_offset = static_cast<uint16_t>(addr & 0xFFFF);
    } else {
      // Non-segmented mode
      pc_offset = static_cast<uint16_t>(addr & 0xFFFF);
      pc_segment = 0;
    }
  }

  // Get stack pointer
  inline uint32_t getSP() const {
    // In non-segmented mode, R15 is the stack pointer
    // In segmented mode, use NSP or SSP depending on mode
    if (fcw & kFCW_SEG) {
      return nsp;  // Normal stack pointer in segmented mode
    } else {
      return r[15];  // R15 in non-segmented mode
    }
  }

  // Get return address (from stack)
  inline uint32_t retaddr() const {
    return getSP();
  }

  // Check if in segmented mode
  inline bool isSegmented() const {
    return (fcw & kFCW_SEG) != 0;
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
  kGDBRegisterR6 = 6,
  kGDBRegisterR7 = 7,
  kGDBRegisterR8 = 8,
  kGDBRegisterR9 = 9,
  kGDBRegisterR10 = 10,
  kGDBRegisterR11 = 11,
  kGDBRegisterR12 = 12,
  kGDBRegisterR13 = 13,
  kGDBRegisterR14 = 14,
  kGDBRegisterR15 = 15,
  kGDBRegisterPC = 16,
  kGDBRegisterFCW = 17,
  kGDBRegisterCount = 18
};

} // namespace Z8000
} // namespace Architecture
} // namespace ds2
