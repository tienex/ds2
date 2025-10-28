//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC SPE (Signal Processing Engine)
//
// SPE is a SIMD extension for embedded PowerPC processors (e500 core)
// as an alternative to AltiVec/VMX. Provides 64-bit SIMD operations.
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace SPE {

//
// SPE Overview
//
// SPE augments the 32-bit GPRs to 64-bit registers for SIMD operations:
// - Upper 32 bits: New accumulator data
// - Lower 32 bits: Original GPR data
//
// This allows dual 32-bit integer operations or single 64-bit operations
// without requiring separate vector register files like AltiVec.
//

// SPE Register State (extends GPRs to 64-bit)
struct SPEState {
  // GPRs are extended to 64-bit when SPE is enabled
  // GPR[i] bits 32-63 contain the upper word
  // GPR[i] bits 0-31 contain the lower word (original 32-bit GPR)
  uint64_t gpr_high[32];  // Upper 32-bit extensions of r0-r31

  // SPE/Embedded Floating-Point Status and Control Register
  uint32_t spefscr;

  // Accumulator for multiply-accumulate operations
  uint64_t acc;  // 64-bit accumulator
};

//
// SPEFSCR (SPE Floating-Point Status and Control Register) bits
//

enum SPEFSCRBits {
  // Summary bits
  SPEFSCR_SOVH     = (1 << 31),  // Summary integer overflow high
  SPEFSCR_OVH      = (1 << 30),  // Integer overflow high
  SPEFSCR_FGH      = (1 << 29),  // Embedded FP guard bit high
  SPEFSCR_FXH      = (1 << 28),  // Embedded FP sticky bit high
  SPEFSCR_FINVH    = (1 << 27),  // Embedded FP invalid operation high
  SPEFSCR_FDBZH    = (1 << 26),  // Embedded FP divide by zero high
  SPEFSCR_FUNFH    = (1 << 25),  // Embedded FP underflow high
  SPEFSCR_FOVFH    = (1 << 24),  // Embedded FP overflow high

  // Rounding mode
  SPEFSCR_MODE     = (1 << 23),  // Embedded FP mode (0=signed, 1=unsigned)
  SPEFSCR_SOV      = (1 << 22),  // Summary integer overflow
  SPEFSCR_OV       = (1 << 21),  // Integer overflow
  SPEFSCR_FG       = (1 << 20),  // Embedded FP guard bit
  SPEFSCR_FX       = (1 << 19),  // Embedded FP sticky bit
  SPEFSCR_FINV     = (1 << 18),  // Embedded FP invalid operation
  SPEFSCR_FDBZ     = (1 << 17),  // Embedded FP divide by zero
  SPEFSCR_FUNF     = (1 << 16),  // Embedded FP underflow
  SPEFSCR_FOVF     = (1 << 15),  // Embedded FP overflow

  // Enable bits
  SPEFSCR_FINXS    = (1 << 14),  // Embedded FP inexact sticky
  SPEFSCR_FINVS    = (1 << 13),  // Embedded FP invalid operation sticky
  SPEFSCR_FDBZS    = (1 << 12),  // Embedded FP divide by zero sticky
  SPEFSCR_FUNFS    = (1 << 11),  // Embedded FP underflow sticky
  SPEFSCR_FOVFS    = (1 << 10),  // Embedded FP overflow sticky
  SPEFSCR_FINXE    = (1 <<  5),  // Embedded FP inexact enable
  SPEFSCR_FINVE    = (1 <<  4),  // Embedded FP invalid op enable
  SPEFSCR_FDBZE    = (1 <<  3),  // Embedded FP divide by zero enable
  SPEFSCR_FUNFE    = (1 <<  2),  // Embedded FP underflow enable
  SPEFSCR_FOVFE    = (1 <<  1),  // Embedded FP overflow enable

  // Rounding mode field (bits 7-8)
  SPEFSCR_FRMC_MASK = 0x3,
  SPEFSCR_FRMC_SHIFT = 7,
};

// Rounding modes
enum RoundingMode {
  ROUND_NEAREST = 0,  // Round to nearest
  ROUND_ZERO    = 1,  // Round toward zero
  ROUND_POSINF  = 2,  // Round toward positive infinity
  ROUND_NEGINF  = 3,  // Round toward negative infinity
};

//
// SPE Instruction Categories
//

enum InstructionCategory {
  // Integer SIMD operations (dual 32-bit or single 64-bit)
  CAT_INTEGER_SIMD,

  // Embedded floating-point operations
  CAT_EMBEDDED_FP,

  // Load/Store operations
  CAT_MEMORY,

  // Accumulator operations
  CAT_ACCUMULATOR,
};

//
// Common SPE Operations
//

namespace Operations {
  // Integer SIMD
  constexpr uint32_t EVADDW       = 0x10000200;  // Vector add word
  constexpr uint32_t EVADDIW      = 0x10000202;  // Vector add immediate word
  constexpr uint32_t EVSUBW       = 0x10000204;  // Vector subtract word
  constexpr uint32_t EVABS        = 0x10000208;  // Vector absolute value
  constexpr uint32_t EVNEG        = 0x10000209;  // Vector negate
  constexpr uint32_t EVMULLW      = 0x10000248;  // Vector multiply low word
  constexpr uint32_t EVMULHW      = 0x10000249;  // Vector multiply high word

  // Shifts and rotates
  constexpr uint32_t EVSRWIS      = 0x10000223;  // Vector shift right word immediate signed
  constexpr uint32_t EVSRWIU      = 0x10000222;  // Vector shift right word immediate unsigned
  constexpr uint32_t EVSLW        = 0x10000224;  // Vector shift left word
  constexpr uint32_t EVRLW        = 0x10000228;  // Vector rotate left word

  // Logical operations
  constexpr uint32_t EVAND        = 0x10000211;  // Vector AND
  constexpr uint32_t EVOR         = 0x10000217;  // Vector OR
  constexpr uint32_t EVXOR        = 0x10000216;  // Vector XOR
  constexpr uint32_t EVANDC       = 0x10000212;  // Vector AND with complement

  // Comparisons
  constexpr uint32_t EVCMPEQ      = 0x10000234;  // Vector compare equal
  constexpr uint32_t EVCMPGTS     = 0x10000231;  // Vector compare greater than signed
  constexpr uint32_t EVCMPGTU     = 0x10000230;  // Vector compare greater than unsigned
  constexpr uint32_t EVCMPLTS     = 0x10000233;  // Vector compare less than signed
  constexpr uint32_t EVCMPLTU     = 0x10000232;  // Vector compare less than unsigned

  // Load/Store
  constexpr uint32_t EVLDD        = 0x10000301;  // Vector load double word
  constexpr uint32_t EVSTDD       = 0x10000321;  // Vector store double word
  constexpr uint32_t EVLDW        = 0x10000302;  // Vector load double into two words
  constexpr uint32_t EVSTDW       = 0x10000322;  // Vector store double from two words

  // Embedded floating-point single precision
  constexpr uint32_t EFSADD       = 0x100002C0;  // FP single add
  constexpr uint32_t EFSSUB       = 0x100002C1;  // FP single subtract
  constexpr uint32_t EFSMUL       = 0x100002C8;  // FP single multiply
  constexpr uint32_t EFSDIV       = 0x100002C9;  // FP single divide
  constexpr uint32_t EFSCMPGT     = 0x100002CC;  // FP single compare greater than
  constexpr uint32_t EFSCMPLT     = 0x100002CD;  // FP single compare less than
  constexpr uint32_t EFSCMPEQ     = 0x100002CE;  // FP single compare equal

  // Embedded floating-point double precision
  constexpr uint32_t EFDADD       = 0x100002E0;  // FP double add
  constexpr uint32_t EFDSUB       = 0x100002E1;  // FP double subtract
  constexpr uint32_t EFDMUL       = 0x100002E8;  // FP double multiply
  constexpr uint32_t EFDDIV       = 0x100002E9;  // FP double divide
  constexpr uint32_t EFDCMPGT     = 0x100002EC;  // FP double compare greater than
  constexpr uint32_t EFDCMPLT     = 0x100002ED;  // FP double compare less than
  constexpr uint32_t EFDCMPEQ     = 0x100002EE;  // FP double compare equal

  // Accumulator operations
  constexpr uint32_t EVMHESSF     = 0x10000403;  // Vector multiply halfwords, even, signed, fractional
  constexpr uint32_t EVMHOSSF     = 0x10000407;  // Vector multiply halfwords, odd, signed, fractional
  constexpr uint32_t EVMHEUMI     = 0x10000408;  // Vector multiply halfwords, even, unsigned, modulo, integer
  constexpr uint32_t EVMHOUMI     = 0x1000040C;  // Vector multiply halfwords, odd, unsigned, modulo, integer
}

//
// MSR bits for SPE
//

enum MSRSPEBits {
  MSR_SPV = (1 << 9),   // SPE Available (SPE/EFP enable)
};

//
// SPE Data Types
//

// Dual 32-bit integer (vector of two 32-bit elements)
union SPE_Vector32 {
  uint64_t dw;          // As 64-bit doubleword
  uint32_t w[2];        // As two 32-bit words [high, low]
  int32_t sw[2];        // As two signed 32-bit words
  uint16_t h[4];        // As four 16-bit halfwords
  int16_t sh[4];        // As four signed 16-bit halfwords
  uint8_t b[8];         // As eight bytes
  int8_t sb[8];         // As eight signed bytes
};

// Check if SPE is available
inline bool IsSPEAvailable(uint32_t msr) {
  return (msr & MSR_SPV) != 0;
}

// Get rounding mode from SPEFSCR
inline RoundingMode GetRoundingMode(uint32_t spefscr) {
  return static_cast<RoundingMode>(
    (spefscr >> SPEFSCR_FRMC_SHIFT) & SPEFSCR_FRMC_MASK
  );
}

//
// SPE-Specific Processor Variants (e500, e600 cores)
//

namespace ProcessorVariants {
  constexpr uint32_t MPC8540      = 0x80200000;  // e500 core
  constexpr uint32_t MPC8541      = 0x80200000;  // e500 core
  constexpr uint32_t MPC8555      = 0x80210000;  // e500 core
  constexpr uint32_t MPC8560      = 0x80210000;  // e500 core
  constexpr uint32_t MPC8548      = 0x80210000;  // e500 core
  constexpr uint32_t P2020        = 0x80210000;  // e500mc core
  constexpr uint32_t P2010        = 0x80210000;  // e500mc core
  constexpr uint32_t P1020        = 0x80210000;  // e500v2 core
}

} // namespace SPE
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
