//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Cray Vector Processor Architecture
//
// Cray-1, Cray X-MP, Cray Y-MP, Cray-2, C90, T90, SV1, etc.
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace Cray {

//
// Cray Vector Processor Overview
//
// Cray supercomputers use a unique 64-bit word-addressed architecture:
// - All data is 64-bit words (no byte addressing in early models)
// - Vector registers hold 64-128 elements of 64-bit words
// - Scalar registers for addressing and control
// - No virtual memory on early models (Cray-1, X-MP)
// - Big-endian architecture
// - Custom instruction set optimized for vector operations
//

//
// Cray CPU State
//

struct CPUState {
  // Vector registers (V0-V7)
  // Each vector register holds 64 elements (Cray-1, X-MP, Y-MP)
  // or 128 elements (C90, T90, SV1)
  union {
    struct {
      uint64_t v0[128], v1[128], v2[128], v3[128];
      uint64_t v4[128], v5[128], v6[128], v7[128];
    };
    uint64_t vr[8][128];
  } vector;

  // Scalar registers (S0-S7) - 64-bit data registers
  union {
    struct {
      uint64_t s0, s1, s2, s3, s4, s5, s6, s7;
    };
    uint64_t s[8];
  } scalar;

  // Address registers (A0-A7) - 24-bit addresses (word addresses, not byte)
  union {
    struct {
      uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
    };
    uint32_t a[8];
  } address;

  // Intermediate address registers (B0-B63)
  // Used for address arithmetic and temporary storage
  union {
    uint32_t b[64];
  } intermediate;

  // Temporary scalar registers (T0-T63)
  // Used for holding intermediate scalar results
  union {
    uint64_t t[64];
  } temporary;

  // Vector length register (VL)
  // Specifies the number of elements to process in vector operations (1-128)
  uint32_t vl;

  // Vector mask register (VM)
  // 128-bit mask (one bit per vector element) for conditional operations
  uint64_t vm[2];  // 128 bits total

  // Program counter (P) - 24-bit word address
  uint32_t pc;

  // Exchange package (for context switching)
  // Contains saved state for job swapping
  struct ExchangePackage {
    uint32_t next_address;      // Address of next instruction
    uint32_t flags;             // Processor flags
    uint64_t s_regs[8];         // Saved S registers
    uint32_t a_regs[8];         // Saved A registers
    uint32_t vl_saved;          // Saved VL
    uint64_t vm_saved[2];       // Saved VM
  } exchange;

  // Processor flags
  struct {
    uint32_t floating_point_error : 1;  // FP error flag
    uint32_t operand_range_error : 1;   // Operand out of range
    uint32_t interrupt_flag : 1;        // Interrupt pending
    uint32_t vector_mode : 1;           // Vector operation in progress
    uint32_t reserved : 28;
  } flags;

  // Functional unit status (Cray-1 has pipelined functional units)
  struct {
    uint32_t vector_add_active : 1;
    uint32_t vector_mul_active : 1;
    uint32_t vector_shift_active : 1;
    uint32_t vector_logical_active : 1;
    uint32_t scalar_add_active : 1;
    uint32_t scalar_mul_active : 1;
    uint32_t scalar_shift_active : 1;
    uint32_t address_add_active : 1;
    uint32_t address_mul_active : 1;
    uint32_t reserved : 23;
  } functional_units;

  // Real-time clock (RTC) - 64-bit counter
  uint64_t rtc;

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

//
// Cray Processor Variants
//

enum class Variant {
  CRAY_1,         // Cray-1 (1976) - First Cray supercomputer, 64 element vectors
  CRAY_1S,        // Cray-1S - Improved Cray-1
  CRAY_1M,        // Cray-1M - Cray-1 with solid-state storage

  CRAY_XMP,       // Cray X-MP (1982) - Multi-processor, 64 element vectors
  CRAY_XMP_EA,    // Cray X-MP EA - Extended addressing

  CRAY_2,         // Cray-2 (1985) - Liquid-cooled, 4 processors
  CRAY_2S,        // Cray-2S - Improved Cray-2

  CRAY_YMP,       // Cray Y-MP (1988) - Faster X-MP successor
  CRAY_YMP_EL,    // Cray Y-MP EL - Entry-level Y-MP

  CRAY_C90,       // Cray C90 (1991) - 128 element vectors, up to 16 processors
  CRAY_T90,       // Cray T90 (1995) - IEEE floating point, faster C90

  CRAY_J90,       // Cray J90 (1994) - Air-cooled, cost-reduced
  CRAY_SV1,       // Cray SV1 (1998) - Last vector supercomputer, 128 element vectors
  CRAY_SV1EX,     // Cray SV1ex - Enhanced SV1

  CRAY_X1,        // Cray X1 (2003) - Vector + MSP (multi-streaming processor)
  CRAY_X1E,       // Cray X1E - Enhanced X1
  CRAY_X2,        // Cray X2 (2007) - Last Cray vector system
};

//
// Cray Instruction Set Features
//

struct CrayFeatures {
  bool has_64_element_vectors;   // Cray-1, X-MP, Y-MP, Cray-2
  bool has_128_element_vectors;  // C90, T90, SV1, X1, X2
  bool has_ieee_fp;              // T90 and later (earlier used Cray FP format)
  bool has_virtual_memory;       // Y-MP and later
  bool has_byte_addressing;      // Later models added byte addressing modes
  bool has_multi_streaming;      // X1/X1E/X2 MSP
  uint32_t max_vector_length;    // 64 or 128
  uint32_t num_processors;       // 1 to 16+ (depends on configuration)
};

// Get features for a specific Cray variant
inline CrayFeatures GetFeatures(Variant variant) {
  CrayFeatures features = {};

  switch (variant) {
    case Variant::CRAY_1:
    case Variant::CRAY_1S:
    case Variant::CRAY_1M:
      features.has_64_element_vectors = true;
      features.max_vector_length = 64;
      features.num_processors = 1;
      features.has_ieee_fp = false;
      features.has_virtual_memory = false;
      break;

    case Variant::CRAY_XMP:
    case Variant::CRAY_XMP_EA:
      features.has_64_element_vectors = true;
      features.max_vector_length = 64;
      features.num_processors = 4;  // Up to 4 processors
      features.has_ieee_fp = false;
      features.has_virtual_memory = false;
      break;

    case Variant::CRAY_2:
    case Variant::CRAY_2S:
      features.has_64_element_vectors = true;
      features.max_vector_length = 64;
      features.num_processors = 4;
      features.has_ieee_fp = false;
      features.has_virtual_memory = false;
      break;

    case Variant::CRAY_YMP:
    case Variant::CRAY_YMP_EL:
      features.has_64_element_vectors = true;
      features.max_vector_length = 64;
      features.num_processors = 8;  // Up to 8 processors
      features.has_ieee_fp = false;
      features.has_virtual_memory = true;
      break;

    case Variant::CRAY_C90:
      features.has_128_element_vectors = true;
      features.max_vector_length = 128;
      features.num_processors = 16;  // Up to 16 processors
      features.has_ieee_fp = false;
      features.has_virtual_memory = true;
      break;

    case Variant::CRAY_T90:
      features.has_128_element_vectors = true;
      features.max_vector_length = 128;
      features.num_processors = 32;  // Up to 32 processors
      features.has_ieee_fp = true;
      features.has_virtual_memory = true;
      break;

    case Variant::CRAY_J90:
      features.has_64_element_vectors = true;
      features.max_vector_length = 64;
      features.num_processors = 1;
      features.has_ieee_fp = true;
      features.has_virtual_memory = true;
      break;

    case Variant::CRAY_SV1:
    case Variant::CRAY_SV1EX:
      features.has_128_element_vectors = true;
      features.max_vector_length = 128;
      features.num_processors = 32;  // Up to 32 processors
      features.has_ieee_fp = true;
      features.has_virtual_memory = true;
      features.has_byte_addressing = true;
      break;

    case Variant::CRAY_X1:
    case Variant::CRAY_X1E:
    case Variant::CRAY_X2:
      features.has_128_element_vectors = true;
      features.max_vector_length = 128;
      features.num_processors = 1;  // Per MSP
      features.has_ieee_fp = true;
      features.has_virtual_memory = true;
      features.has_byte_addressing = true;
      features.has_multi_streaming = true;
      break;
  }

  return features;
}

//
// Cray Instruction Formats
//

// Cray instructions are 16-bit or 32-bit parcels packed into 64-bit words
// A word can contain 4 16-bit parcels or 2 32-bit parcels

enum class InstructionFormat {
  PARCEL_16,  // 16-bit instruction parcel
  PARCEL_32,  // 32-bit instruction parcel
};

// 16-bit instruction parcel format
struct Instruction16 {
  uint16_t opcode : 7;    // Operation code
  uint16_t i : 3;         // Register field i
  uint16_t j : 3;         // Register field j
  uint16_t k : 3;         // Register field k
};

// 32-bit instruction parcel format
struct Instruction32 {
  uint32_t opcode : 7;    // Operation code
  uint32_t i : 3;         // Register field i
  uint32_t j : 3;         // Register field j
  uint32_t k : 3;         // Register field k
  uint32_t addr : 16;     // Address or immediate value
};

//
// Common Cray Instructions
//

namespace Instructions {
  // Vector arithmetic
  constexpr uint16_t V_ADD    = 0x60;  // Vector add
  constexpr uint16_t V_SUB    = 0x61;  // Vector subtract
  constexpr uint16_t V_MUL    = 0x62;  // Vector multiply
  constexpr uint16_t V_DIV    = 0x63;  // Vector divide (reciprocal approximation)

  // Vector logical
  constexpr uint16_t V_AND    = 0x64;  // Vector AND
  constexpr uint16_t V_OR     = 0x65;  // Vector OR
  constexpr uint16_t V_XOR    = 0x66;  // Vector XOR
  constexpr uint16_t V_SHIFT  = 0x67;  // Vector shift

  // Scalar arithmetic
  constexpr uint16_t S_ADD    = 0x30;  // Scalar add
  constexpr uint16_t S_SUB    = 0x31;  // Scalar subtract
  constexpr uint16_t S_MUL    = 0x32;  // Scalar multiply
  constexpr uint16_t S_DIV    = 0x33;  // Scalar reciprocal approximation

  // Address arithmetic
  constexpr uint16_t A_ADD    = 0x10;  // Address add
  constexpr uint16_t A_SUB    = 0x11;  // Address subtract
  constexpr uint16_t A_MUL    = 0x12;  // Address multiply

  // Memory operations
  constexpr uint16_t V_LOAD   = 0x70;  // Vector load
  constexpr uint16_t V_STORE  = 0x71;  // Vector store
  constexpr uint16_t S_LOAD   = 0x40;  // Scalar load
  constexpr uint16_t S_STORE  = 0x41;  // Scalar store

  // Control flow
  constexpr uint16_t JUMP     = 0x04;  // Unconditional jump
  constexpr uint16_t JUMP_Z   = 0x05;  // Jump if zero
  constexpr uint16_t JUMP_NZ  = 0x06;  // Jump if non-zero
  constexpr uint16_t JUMP_POS = 0x07;  // Jump if positive
  constexpr uint16_t JUMP_NEG = 0x08;  // Jump if negative

  // Exchange (context switch)
  constexpr uint16_t EXCHANGE = 0x02;  // Exchange (job swap)
}

//
// Cray Floating-Point Format (non-IEEE)
//

struct CrayFloat {
  // Cray floating point: 64-bit word
  // Bit 63: Sign
  // Bits 62-48: Exponent (15 bits, biased by 16384)
  // Bits 47-0: Mantissa (48 bits, normalized)

  uint64_t mantissa : 48;
  uint64_t exponent : 15;
  uint64_t sign : 1;

  // Convert to IEEE 754 double
  double toIEEE() const;

  // Convert from IEEE 754 double
  static CrayFloat fromIEEE(double value);
};

//
// Debugging Support
//

// Breakpoint instruction (implementation specific - often a reserved instruction)
constexpr uint16_t BREAKPOINT = 0x0000;  // Reserved instruction used for breakpoint

} // namespace Cray
} // namespace Architecture
} // namespace ds2
