//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC VLE (Variable Length Encoding)
//
// VLE provides 16-bit and 32-bit instruction encoding for improved code density
// in embedded PowerPC applications (e200, e500mc, Qorivva, etc.)
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace VLE {

//
// VLE Instruction Encoding
//
// VLE instructions can be either 16-bit or 32-bit:
// - 16-bit instructions: Most common operations with limited operand ranges
// - 32-bit instructions: Full-featured operations
//
// VLE is incompatible with standard PowerPC instructions and requires
// the processor to be in VLE mode (controlled by MSR[CM] or segment attributes)
//

// VLE instruction format detection
enum class InstructionFormat {
  VLE_16BIT,     // 16-bit VLE instruction
  VLE_32BIT,     // 32-bit VLE instruction
  STANDARD_32BIT // Standard PowerPC 32-bit instruction
};

// Determine instruction format from opcode
inline InstructionFormat GetInstructionFormat(uint16_t opcode, bool vle_mode) {
  if (!vle_mode) {
    return InstructionFormat::STANDARD_32BIT;
  }

  // VLE 16-bit instruction primary opcodes (simplified detection)
  // Opcodes 0-31 (binary 000000-011111) are typically 16-bit in VLE mode
  uint8_t primary_opcode = (opcode >> 10) & 0x3F;

  // 16-bit VLE instructions have primary opcode in specific ranges
  if (primary_opcode <= 0x1F) {
    return InstructionFormat::VLE_16BIT;
  }

  return InstructionFormat::VLE_32BIT;
}

// Get instruction size in bytes
inline size_t GetInstructionSize(uint16_t opcode, bool vle_mode) {
  InstructionFormat format = GetInstructionFormat(opcode, vle_mode);
  return (format == InstructionFormat::VLE_16BIT) ? 2 : 4;
}

//
// VLE 16-bit Instruction Formats
//

// Format: se_illegal (0x0000)
struct VLE_ILLEGAL {
  static constexpr uint16_t OPCODE = 0x0000;
};

// Format: se_b target (branch)
struct VLE_B {
  uint16_t bd : 10;    // Branch displacement (-512 to +511 * 2 bytes)
  uint16_t opcode : 6; // 0x38 (111000)

  int32_t GetTarget() const {
    // Sign-extend 10-bit displacement and multiply by 2
    int32_t disp = (bd & 0x200) ? (bd | 0xFFFFFC00) : bd;
    return disp << 1;
  }
};

// Format: se_bc target (conditional branch)
struct VLE_BC {
  uint16_t bd : 5;     // Branch displacement (-16 to +15 * 2 bytes)
  uint16_t cr : 3;     // CR field
  uint16_t bo : 2;     // Branch condition
  uint16_t opcode : 6; // 0x3A (111010)

  int32_t GetTarget() const {
    int32_t disp = (bd & 0x10) ? (bd | 0xFFFFFFE0) : bd;
    return disp << 1;
  }
};

// Format: se_blr (branch to link register)
struct VLE_BLR {
  uint16_t reserved : 10;
  uint16_t opcode : 6; // 0x01 (000001)
};

// Format: se_add rX, rY (add registers)
struct VLE_ADD {
  uint16_t ry : 4;     // Source register Y
  uint16_t rx : 4;     // Source/Destination register X
  uint16_t subop : 2;  // Sub-opcode
  uint16_t opcode : 6; // 0x02 (000010)
};

// Format: se_li rX, UI5 (load immediate 5-bit unsigned)
struct VLE_LI {
  uint16_t ui5 : 5;    // 5-bit unsigned immediate (0-31)
  uint16_t rx : 4;     // Destination register
  uint16_t reserved : 1;
  uint16_t opcode : 6; // 0x09 (001001)
};

// Format: se_cmpi rX, UI5 (compare immediate)
struct VLE_CMPI {
  uint16_t ui5 : 5;    // 5-bit unsigned immediate
  uint16_t rx : 4;     // Source register
  uint16_t reserved : 1;
  uint16_t opcode : 6; // 0x0A (001010)
};

// Format: se_bmaski rX, UI5 (bit mask immediate)
struct VLE_BMASKI {
  uint16_t ui5 : 5;    // Mask bit count
  uint16_t rx : 4;     // Destination register
  uint16_t reserved : 1;
  uint16_t opcode : 6; // 0x0B (001011)
};

// Format: se_lwz rX, offset(rY) (load word and zero)
struct VLE_LWZ {
  uint16_t d : 4;      // Displacement in words (0-60 bytes)
  uint16_t rx : 4;     // Destination register
  uint16_t ry : 4;     // Base register
  uint16_t opcode : 4; // 0xC (1100)
};

// Format: se_stw rX, offset(rY) (store word)
struct VLE_STW {
  uint16_t d : 4;      // Displacement in words (0-60 bytes)
  uint16_t rx : 4;     // Source register
  uint16_t ry : 4;     // Base register
  uint16_t opcode : 4; // 0xD (1101)
};

//
// VLE 32-bit Instruction Formats
//

// Format: e_add16i rD, rA, SI (add 16-bit signed immediate)
struct VLE32_ADD16I {
  uint32_t si : 16;    // 16-bit signed immediate
  uint32_t ra : 5;     // Source register A
  uint32_t rd : 5;     // Destination register
  uint32_t opcode : 6; // Primary opcode
};

// Format: e_lis rD, value (load immediate shifted)
struct VLE32_LIS {
  uint32_t value : 16; // Upper 16-bit immediate
  uint32_t rd : 5;     // Destination register
  uint32_t subop : 5;
  uint32_t opcode : 6; // Primary opcode
};

// Format: e_b target (branch)
struct VLE32_B {
  uint32_t bd : 24;    // 24-bit signed branch displacement
  uint32_t lk : 1;     // Link bit
  uint32_t opcode : 7; // Primary opcode

  int32_t GetTarget() const {
    // Sign-extend 24-bit displacement and multiply by 2
    int32_t disp = (bd & 0x800000) ? (bd | 0xFF000000) : bd;
    return disp << 1;
  }
};

// Format: e_bc target (conditional branch)
struct VLE32_BC {
  uint32_t bd : 16;    // 16-bit signed branch displacement
  uint32_t bi : 5;     // Condition bit
  uint32_t bo : 5;     // Branch condition
  uint32_t lk : 1;     // Link bit
  uint32_t opcode : 5; // Primary opcode

  int32_t GetTarget() const {
    int32_t disp = (bd & 0x8000) ? (bd | 0xFFFF0000) : bd;
    return disp << 1;
  }
};

// Format: e_lwz rD, D(rA) (load word and zero)
struct VLE32_LWZ {
  uint32_t d : 16;     // Displacement
  uint32_t ra : 5;     // Base register
  uint32_t rd : 5;     // Destination register
  uint32_t opcode : 6; // Primary opcode
};

// Format: e_stw rS, D(rA) (store word)
struct VLE32_STW {
  uint32_t d : 16;     // Displacement
  uint32_t ra : 5;     // Base register
  uint32_t rs : 5;     // Source register
  uint32_t opcode : 6; // Primary opcode
};

//
// VLE Opcode Definitions
//

namespace Opcodes {
  // 16-bit VLE opcodes (6-bit primary opcode field)
  namespace SE {
    constexpr uint16_t ILLEGAL   = 0x00;  // 000000
    constexpr uint16_t ISYNC     = 0x01;  // 000001
    constexpr uint16_t SC        = 0x02;  // 000010
    constexpr uint16_t BLRL      = 0x03;  // 000011
    constexpr uint16_t BCTR      = 0x04;  // 000100
    constexpr uint16_t BCTRL     = 0x05;  // 000101
    constexpr uint16_t RFI       = 0x06;  // 000110
    constexpr uint16_t RFCI      = 0x07;  // 000111
    constexpr uint16_t RFDI      = 0x08;  // 001000
    constexpr uint16_t RFMCI     = 0x09;  // 001001
    constexpr uint16_t NOT       = 0x0A;  // 001010
    constexpr uint16_t NEG       = 0x0B;  // 001011
    constexpr uint16_t MFCTR     = 0x0C;  // 001100
    constexpr uint16_t MTCTR     = 0x0D;  // 001101
    constexpr uint16_t EXTSB     = 0x0E;  // 001110
    constexpr uint16_t EXTSH     = 0x0F;  // 001111
    constexpr uint16_t EXTZB     = 0x10;  // 010000
    constexpr uint16_t EXTZH     = 0x11;  // 010001
    constexpr uint16_t ABS       = 0x12;  // 010010
    constexpr uint16_t B         = 0x38;  // 111000
    constexpr uint16_t BL        = 0x39;  // 111001
    constexpr uint16_t BC        = 0x3A;  // 111010
  }

  // 32-bit VLE opcodes (5-7 bit primary opcode)
  namespace E {
    constexpr uint32_t ADD16I    = 0x1C;  // Extended add immediate
    constexpr uint32_t LIS       = 0x1C;  // Load immediate shifted
    constexpr uint32_t AND2I     = 0x1C;  // AND with immediate
    constexpr uint32_t OR2I      = 0x1C;  // OR with immediate
    constexpr uint32_t B         = 0x3E;  // Branch
    constexpr uint32_t BC        = 0x3D;  // Conditional branch
    constexpr uint32_t LWZ       = 0x30;  // Load word and zero
    constexpr uint32_t STW       = 0x34;  // Store word
    constexpr uint32_t LBZ       = 0x31;  // Load byte and zero
    constexpr uint32_t STB       = 0x35;  // Store byte
    constexpr uint32_t LHZ       = 0x32;  // Load halfword and zero
    constexpr uint32_t STH       = 0x36;  // Store halfword
    constexpr uint32_t LMVSPRG   = 0x39;  // Load multiple volatile SPR
    constexpr uint32_t STMVSPRG  = 0x3A;  // Store multiple volatile SPR
  }
}

//
// VLE Mode Control
//

// MSR bits for VLE control
enum MSRBits {
  MSR_CM = (1 << 7),   // Computation Mode (VLE when set on some processors)
};

// Check if address is in VLE segment (implementation specific)
inline bool IsVLEAddress(uint32_t address) {
  // VLE segments are typically configured via MMU/MPU
  // This is processor and OS specific
  // Return false by default - must be configured per platform
  return false;
}

//
// VLE Instruction Disassembly Helpers
//

struct VLEInstruction {
  union {
    uint16_t raw16;
    uint32_t raw32;

    // 16-bit formats
    VLE_B se_b;
    VLE_BC se_bc;
    VLE_BLR se_blr;
    VLE_ADD se_add;
    VLE_LI se_li;
    VLE_CMPI se_cmpi;
    VLE_BMASKI se_bmaski;
    VLE_LWZ se_lwz;
    VLE_STW se_stw;

    // 32-bit formats
    VLE32_ADD16I e_add16i;
    VLE32_LIS e_lis;
    VLE32_B e_b;
    VLE32_BC e_bc;
    VLE32_LWZ e_lwz;
    VLE32_STW e_stw;
  };

  InstructionFormat format;

  VLEInstruction(uint16_t word, bool vle_mode) {
    raw16 = word;
    format = GetInstructionFormat(word, vle_mode);
  }

  size_t GetSize() const {
    return (format == InstructionFormat::VLE_16BIT) ? 2 : 4;
  }

  bool IsVLE() const {
    return format != InstructionFormat::STANDARD_32BIT;
  }
};

// Breakpoint instructions
constexpr uint16_t VLE_BREAKPOINT_16 = 0x0000;  // se_illegal
constexpr uint32_t VLE_BREAKPOINT_32 = 0x7C000000;  // Can use e_trap or standard tw

} // namespace VLE
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
