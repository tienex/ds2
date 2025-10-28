//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Clipper Architecture
//
// 32-bit RISC processor developed by Fairchild Semiconductor
// Used in Intergraph workstations running CLIX (Unix System V variant)
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace Clipper {

//
// Clipper Architecture Overview
//
// The Clipper was a 32-bit RISC architecture that:
// - Developed by Fairchild Semiconductor (1985-1989)
// - Used primarily by Intergraph in their workstations
// - 32-bit data and address paths
// - 16 general purpose registers
// - Hardware floating-point support
// - Memory-mapped I/O
// - Virtual memory with TLB
// - Little-endian byte order
//

//
// Clipper CPU State
//

struct CPUState {
  // General purpose registers (R0-R15)
  // R14 = Frame Pointer (FP)
  // R15 = Stack Pointer (SP)
  union {
    uint32_t regs[16];
    struct {
      uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
      uint32_t r8, r9, r10, r11, r12, r13, r14_fp, r15_sp;
    };
    struct {
      uint32_t r0_ret, r1_arg1, r2_arg2, r3_arg3;
      uint32_t r4, r5, r6, r7, r8, r9, r10, r11, r12, r13;
      uint32_t fp, sp;
    };
  } gp;

  // Program counter
  uint32_t pc;

  // Processor Status Word (PSW)
  union {
    uint32_t psw;
    struct {
      uint32_t c : 1;       // Carry flag
      uint32_t v : 1;       // Overflow flag
      uint32_t z : 1;       // Zero flag
      uint32_t n : 1;       // Negative flag
      uint32_t t : 1;       // Trace enable
      uint32_t k : 1;       // Kernel mode
      uint32_t u : 1;       // User mode
      uint32_t s : 1;       // Supervisor mode
      uint32_t i : 1;       // Interrupt enable
      uint32_t reserved : 23;
    };
  } psw;

  // System Status Word (SSW) - system-level status
  uint32_t ssw;

  // Floating-point registers (F0-F7)
  // 64-bit IEEE 754 double precision
  union {
    double regs[8];
    struct {
      double f0, f1, f2, f3, f4, f5, f6, f7;
    };
  } fp;

  // Floating-point status register
  uint32_t fpsr;

  // MMU registers
  struct {
    uint32_t ptb0;      // Page table base 0 (user)
    uint32_t ptb1;      // Page table base 1 (supervisor)
    uint32_t ptb2;      // Page table base 2 (kernel)
    uint32_t ptlr;      // Page table length register
    uint32_t fault_addr; // Fault address register
  } mmu;

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

//
// Clipper Processor Variants
//

enum class Variant {
  C100,       // Clipper C100 - First generation (1985)
  C300,       // Clipper C300 - Second generation (1987)
  C400,       // Clipper C400 - Third generation (1989)
  C400_PLUS,  // Clipper C400+ - Enhanced C400
};

//
// Clipper Features
//

struct ClipperFeatures {
  bool has_fpu;           // Floating-point unit
  bool has_mmu;           // Memory management unit
  uint32_t clock_speed;   // Clock speed in MHz
  uint32_t cache_size;    // Cache size in KB
};

// Get features for specific Clipper variant
inline ClipperFeatures GetFeatures(Variant variant) {
  ClipperFeatures features = {};

  switch (variant) {
    case Variant::C100:
      features.has_fpu = true;
      features.has_mmu = true;
      features.clock_speed = 33;   // 33 MHz
      features.cache_size = 4;     // 4 KB
      break;

    case Variant::C300:
      features.has_fpu = true;
      features.has_mmu = true;
      features.clock_speed = 50;   // 50 MHz
      features.cache_size = 8;     // 8 KB
      break;

    case Variant::C400:
      features.has_fpu = true;
      features.has_mmu = true;
      features.clock_speed = 66;   // 66 MHz
      features.cache_size = 16;    // 16 KB
      break;

    case Variant::C400_PLUS:
      features.has_fpu = true;
      features.has_mmu = true;
      features.clock_speed = 100;  // 100 MHz
      features.cache_size = 32;    // 32 KB
      break;
  }

  return features;
}

//
// Clipper Instruction Formats
//

// Clipper instructions are 16-bit or 32-bit (most are 32-bit)

enum InstructionFormat {
  FORMAT_SHORT,   // 16-bit instruction
  FORMAT_LONG,    // 32-bit instruction
};

// 32-bit instruction format
struct Instruction32 {
  uint32_t opcode : 6;    // Operation code
  uint32_t rd : 4;        // Destination register
  uint32_t rs1 : 4;       // Source register 1
  uint32_t rs2 : 4;       // Source register 2
  uint32_t mode : 3;      // Addressing mode
  uint32_t reserved : 11;
};

// 16-bit instruction format
struct Instruction16 {
  uint16_t opcode : 4;    // Operation code
  uint16_t rd : 4;        // Destination register
  uint16_t imm : 8;       // Immediate value
};

//
// Clipper Instruction Opcodes (subset)
//

namespace Opcodes {
  // Load/Store
  constexpr uint32_t LOADW    = 0x01;  // Load word
  constexpr uint32_t LOADB    = 0x02;  // Load byte
  constexpr uint32_t LOADH    = 0x03;  // Load halfword
  constexpr uint32_t STOREW   = 0x04;  // Store word
  constexpr uint32_t STOREB   = 0x05;  // Store byte
  constexpr uint32_t STOREH   = 0x06;  // Store halfword

  // Arithmetic
  constexpr uint32_t ADD      = 0x10;  // Add
  constexpr uint32_t ADDC     = 0x11;  // Add with carry
  constexpr uint32_t SUB      = 0x12;  // Subtract
  constexpr uint32_t SUBC     = 0x13;  // Subtract with carry
  constexpr uint32_t MUL      = 0x14;  // Multiply
  constexpr uint32_t DIV      = 0x15;  // Divide
  constexpr uint32_t NEG      = 0x16;  // Negate
  constexpr uint32_t ABS      = 0x17;  // Absolute value

  // Logical
  constexpr uint32_t AND      = 0x20;  // AND
  constexpr uint32_t OR       = 0x21;  // OR
  constexpr uint32_t XOR      = 0x22;  // XOR
  constexpr uint32_t NOT      = 0x23;  // NOT
  constexpr uint32_t SHL      = 0x24;  // Shift left
  constexpr uint32_t SHR      = 0x25;  // Shift right
  constexpr uint32_t SAR      = 0x26;  // Shift arithmetic right
  constexpr uint32_t ROL      = 0x27;  // Rotate left
  constexpr uint32_t ROR      = 0x28;  // Rotate right

  // Compare
  constexpr uint32_t CMP      = 0x30;  // Compare
  constexpr uint32_t CMPU     = 0x31;  // Compare unsigned

  // Branch
  constexpr uint32_t BR       = 0x40;  // Branch unconditional
  constexpr uint32_t BEQ      = 0x41;  // Branch if equal
  constexpr uint32_t BNE      = 0x42;  // Branch if not equal
  constexpr uint32_t BLT      = 0x43;  // Branch if less than
  constexpr uint32_t BLE      = 0x44;  // Branch if less than or equal
  constexpr uint32_t BGT      = 0x45;  // Branch if greater than
  constexpr uint32_t BGE      = 0x46;  // Branch if greater than or equal

  // Jump
  constexpr uint32_t CALL     = 0x50;  // Call procedure
  constexpr uint32_t RET      = 0x51;  // Return from procedure
  constexpr uint32_t JMP      = 0x52;  // Jump

  // Floating-point
  constexpr uint32_t FADD     = 0x60;  // FP add
  constexpr uint32_t FSUB     = 0x61;  // FP subtract
  constexpr uint32_t FMUL     = 0x62;  // FP multiply
  constexpr uint32_t FDIV     = 0x63;  // FP divide
  constexpr uint32_t FSQRT    = 0x64;  // FP square root
  constexpr uint32_t FCMP     = 0x65;  // FP compare
  constexpr uint32_t FLOAD    = 0x66;  // FP load
  constexpr uint32_t FSTORE   = 0x67;  // FP store

  // System
  constexpr uint32_t TRAP     = 0x70;  // Trap (software interrupt)
  constexpr uint32_t LOADPSW  = 0x71;  // Load PSW
  constexpr uint32_t STOREPSW = 0x72;  // Store PSW
  constexpr uint32_t RFE      = 0x73;  // Return from exception
  constexpr uint32_t HALT     = 0x74;  // Halt processor
  constexpr uint32_t NOP      = 0x00;  // No operation

  // TLB operations
  constexpr uint32_t TLBREAD  = 0x7A;  // TLB read
  constexpr uint32_t TLBWRITE = 0x7B;  // TLB write
  constexpr uint32_t TLBINVAL = 0x7C;  // TLB invalidate
}

//
// Condition Codes
//

enum ConditionCodes {
  CC_C = 0x01,  // Carry
  CC_V = 0x02,  // Overflow
  CC_Z = 0x04,  // Zero
  CC_N = 0x08,  // Negative
};

//
// Addressing Modes
//

enum AddressingMode {
  MODE_REGISTER,          // Register direct
  MODE_REGISTER_INDIRECT, // Register indirect
  MODE_INDEXED,           // Indexed
  MODE_PC_RELATIVE,       // PC-relative
  MODE_IMMEDIATE,         // Immediate
  MODE_ABSOLUTE,          // Absolute
};

//
// Exception Types
//

enum ExceptionType {
  EXC_RESET        = 0,   // Reset
  EXC_BUS_ERROR    = 1,   // Bus error
  EXC_ADDRESS      = 2,   // Address error
  EXC_ILLEGAL_INSN = 3,   // Illegal instruction
  EXC_DIVIDE_ZERO  = 4,   // Divide by zero
  EXC_PRIVILEGE    = 5,   // Privilege violation
  EXC_TRACE        = 6,   // Trace
  EXC_BREAKPOINT   = 7,   // Breakpoint
  EXC_OVERFLOW     = 8,   // Integer overflow
  EXC_FP_EXCEPTION = 9,   // Floating-point exception
  EXC_TLB_MISS     = 10,  // TLB miss
  EXC_PAGE_FAULT   = 11,  // Page fault
  EXC_INTERRUPT    = 12,  // External interrupt
};

// Breakpoint instruction
constexpr uint32_t BREAKPOINT = Opcodes::TRAP;  // TRAP instruction used for breakpoints

} // namespace Clipper
} // namespace Architecture
} // namespace ds2
