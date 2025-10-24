//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Tahoe (CCI Power 6/32) Architecture
//
// The Tahoe was a 32-bit CISC processor from Computer Consoles Inc. (CCI)
// Used in CCI Power 6/32 systems and Harris HCX-9 systems
// VAX-like instruction set but simplified and distinct
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace Tahoe {

//
// Tahoe CPU Architecture Overview
//
// The Tahoe (CCI Power 6/32) was a 32-bit CISC architecture that:
// - Had a VAX-like instruction set but was not VAX-compatible
// - Used in Computer Consoles Inc. (CCI) Power 6/32 computers
// - Used in Harris HCX-9 fault-tolerant systems
// - 4.3BSD-Tahoe was named after this architecture
// - 16 general purpose 32-bit registers
// - Memory-to-memory operations like VAX
// - Big-endian byte order
//

//
// Tahoe CPU State
//

struct CPUState {
  // General purpose registers (R0-R15)
  // R14 = FP (Frame Pointer)
  // R15 = SP (Stack Pointer)
  union {
    uint32_t regs[16];
    struct {
      uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
      uint32_t r8, r9, r10, r11, r12, r13, r14_fp, r15_sp;
    };
    struct {
      uint32_t r0_ret, r1_ret2, r2_arg, r3_arg, r4_arg, r5_arg;
      uint32_t r6, r7, r8, r9, r10, r11, r12, r13;
      uint32_t fp, sp;
    };
  } gp;

  // Program counter (PC)
  uint32_t pc;

  // Processor Status Longword (PSL)
  union {
    uint32_t psl;
    struct {
      uint32_t c : 1;       // Carry flag
      uint32_t v : 1;       // Overflow flag
      uint32_t z : 1;       // Zero flag
      uint32_t n : 1;       // Negative flag
      uint32_t t : 1;       // Trace enable
      uint32_t reserved1 : 3;
      uint32_t ipl : 5;     // Interrupt priority level (0-31)
      uint32_t reserved2 : 3;
      uint32_t prvmod : 2;  // Previous mode (kernel/exec/super/user)
      uint32_t curmod : 2;  // Current mode
      uint32_t is : 1;      // Interrupt stack
      uint32_t cm : 1;      // Compatibility mode
      uint32_t tp : 1;      // Trace pending
      uint32_t reserved3 : 9;
    };
  } psl;

  // Argument pointer (AP) - for procedure calling
  uint32_t ap;

  // Floating-point registers (optional FPU)
  // Tahoe had optional floating-point support
  double ac[4];             // Accumulator registers AC0-AC3

  // Floating-point status register
  uint32_t fpsr;

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

//
// Tahoe Instruction Formats
//

// Tahoe instructions are variable length (1 to many bytes)
// Similar to VAX but simplified

enum OperandType {
  OP_REGISTER,          // Register operand (Rn)
  OP_REGISTER_DEFERRED, // Register deferred @(Rn)
  OP_AUTOINCREMENT,     // Autoincrement (Rn)+
  OP_AUTODECREMENT,     // Autodecrement -(Rn)
  OP_DISPLACEMENT,      // Displacement d(Rn)
  OP_IMMEDIATE,         // Immediate #literal
  OP_ABSOLUTE,          // Absolute @#address
  OP_INDEXED,           // Indexed [Rx]
};

// Instruction opcodes (subset)
namespace Opcodes {
  // Data movement
  constexpr uint8_t MOVB   = 0x90;  // Move byte
  constexpr uint8_t MOVW   = 0xB0;  // Move word (16-bit)
  constexpr uint8_t MOVL   = 0xD0;  // Move longword (32-bit)
  constexpr uint8_t CLRB   = 0x94;  // Clear byte
  constexpr uint8_t CLRW   = 0xB4;  // Clear word
  constexpr uint8_t CLRL   = 0xD4;  // Clear longword

  // Arithmetic
  constexpr uint8_t ADDB2  = 0x80;  // Add byte 2 operand
  constexpr uint8_t ADDB3  = 0x81;  // Add byte 3 operand
  constexpr uint8_t ADDW2  = 0xA0;  // Add word 2 operand
  constexpr uint8_t ADDW3  = 0xA1;  // Add word 3 operand
  constexpr uint8_t ADDL2  = 0xC0;  // Add longword 2 operand
  constexpr uint8_t ADDL3  = 0xC1;  // Add longword 3 operand
  constexpr uint8_t SUBB2  = 0x82;  // Subtract byte 2 operand
  constexpr uint8_t SUBB3  = 0x83;  // Subtract byte 3 operand
  constexpr uint8_t SUBW2  = 0xA2;  // Subtract word 2 operand
  constexpr uint8_t SUBW3  = 0xA3;  // Subtract word 3 operand
  constexpr uint8_t SUBL2  = 0xC2;  // Subtract longword 2 operand
  constexpr uint8_t SUBL3  = 0xC3;  // Subtract longword 3 operand
  constexpr uint8_t MULB2  = 0x84;  // Multiply byte
  constexpr uint8_t MULW2  = 0xA4;  // Multiply word
  constexpr uint8_t MULL2  = 0xC4;  // Multiply longword
  constexpr uint8_t DIVB2  = 0x86;  // Divide byte
  constexpr uint8_t DIVW2  = 0xA6;  // Divide word
  constexpr uint8_t DIVL2  = 0xC6;  // Divide longword

  // Logical
  constexpr uint8_t BITB   = 0x93;  // Bit test byte
  constexpr uint8_t BITW   = 0xB3;  // Bit test word
  constexpr uint8_t BITL   = 0xD3;  // Bit test longword
  constexpr uint8_t BISB2  = 0x88;  // Bit set byte
  constexpr uint8_t BISW2  = 0xA8;  // Bit set word
  constexpr uint8_t BISL2  = 0xC8;  // Bit set longword
  constexpr uint8_t BICB2  = 0x8A;  // Bit clear byte
  constexpr uint8_t BICW2  = 0xAA;  // Bit clear word
  constexpr uint8_t BICL2  = 0xCA;  // Bit clear longword
  constexpr uint8_t XORB2  = 0x8C;  // XOR byte
  constexpr uint8_t XORW2  = 0xAC;  // XOR word
  constexpr uint8_t XORL2  = 0xCC;  // XOR longword

  // Compare
  constexpr uint8_t CMPB   = 0x91;  // Compare byte
  constexpr uint8_t CMPW   = 0xB1;  // Compare word
  constexpr uint8_t CMPL   = 0xD1;  // Compare longword

  // Shift
  constexpr uint8_t ASHL   = 0x78;  // Arithmetic shift left
  constexpr uint8_t ASHQ   = 0x79;  // Arithmetic shift quadword

  // Branch
  constexpr uint8_t BRB    = 0x11;  // Branch unconditional (byte displacement)
  constexpr uint8_t BRW    = 0x31;  // Branch unconditional (word displacement)
  constexpr uint8_t BEQL   = 0x13;  // Branch if equal
  constexpr uint8_t BNEQ   = 0x12;  // Branch if not equal
  constexpr uint8_t BGTR   = 0x14;  // Branch if greater than
  constexpr uint8_t BLEQ   = 0x15;  // Branch if less than or equal
  constexpr uint8_t BGEQ   = 0x18;  // Branch if greater than or equal
  constexpr uint8_t BLSS   = 0x19;  // Branch if less than
  constexpr uint8_t BGTRU  = 0x1A;  // Branch if greater than (unsigned)
  constexpr uint8_t BLEQU  = 0x1B;  // Branch if less than or equal (unsigned)

  // Procedure call
  constexpr uint8_t CALLS  = 0xFB;  // Call procedure (stack)
  constexpr uint8_t CALLF  = 0xFE;  // Call procedure (frame)
  constexpr uint8_t RET    = 0x04;  // Return from procedure

  // Jump
  constexpr uint8_t JMP    = 0x17;  // Jump

  // Miscellaneous
  constexpr uint8_t NOP    = 0x01;  // No operation
  constexpr uint8_t HALT   = 0x00;  // Halt processor
  constexpr uint8_t BPT    = 0x03;  // Breakpoint
  constexpr uint8_t PUSHL  = 0xDD;  // Push longword
  constexpr uint8_t POPL   = 0xDC;  // Pop longword

  // String operations
  constexpr uint8_t MOVC3  = 0x28;  // Move character 3 operand
  constexpr uint8_t CMPC3  = 0x29;  // Compare character 3 operand

  // Special
  constexpr uint8_t LDPCTX = 0x06;  // Load process context
  constexpr uint8_t SVPCTX = 0x07;  // Save process context
}

//
// Condition codes
//

enum ConditionCodes {
  CC_C = 0x01,  // Carry
  CC_V = 0x02,  // Overflow
  CC_Z = 0x04,  // Zero
  CC_N = 0x08,  // Negative
};

//
// Processor modes
//

enum ProcessorMode {
  MODE_KERNEL = 0,      // Kernel mode
  MODE_EXECUTIVE = 1,   // Executive mode
  MODE_SUPERVISOR = 2,  // Supervisor mode
  MODE_USER = 3,        // User mode
};

//
// Interrupt Priority Levels
//

constexpr uint32_t IPL_MIN = 0;   // Minimum IPL
constexpr uint32_t IPL_MAX = 31;  // Maximum IPL

//
// Register Conventions
//

// Register usage in procedure calls:
// R0, R1 - Return values
// R2-R5 - Arguments (also on stack for more args)
// R6-R13 - Saved registers (callee-saved)
// R14 (FP) - Frame pointer
// R15 (SP) - Stack pointer

// Breakpoint instruction
constexpr uint8_t BREAKPOINT = Opcodes::BPT;

} // namespace Tahoe
} // namespace Architecture
} // namespace ds2
