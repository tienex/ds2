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

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace MIPS {

// MIPS Application Binary Interfaces (ABIs)
enum class ABI {
  Unknown = 0,

  // O32 - Original 32-bit ABI
  // - Used on MIPS32 (32-bit pointers)
  // - 4 argument registers (a0-a3)
  // - Rest of arguments on stack
  // - System calls use register v0 for syscall number
  O32,

  // N32 - New 32-bit ABI for MIPS64
  // - 32-bit pointers on MIPS64 hardware
  // - 8 argument registers (a0-a7)
  // - 64-bit GPRs but 32-bit addresses
  // - More efficient than O32
  N32,

  // N64 - New 64-bit ABI for MIPS64
  // - 64-bit pointers
  // - 8 argument registers (a0-a7)
  // - Full 64-bit addressing
  N64,

  // O64 - Original 64-bit ABI (rarely used)
  // - 64-bit pointers
  // - 4 argument registers (a0-a3) like O32
  // - Mostly obsolete, replaced by N64
  O64,

  // EABI - Embedded ABI
  // - Used in embedded systems
  // - Optimized for code size
  // - Variations: EABI32, EABI64
  EABI,

  // NUBI - New ABI (experimental)
  // - Proposed improvements over N32/N64
  // - Not widely adopted
  NUBI,
};

// ABI calling convention details
struct ABIInfo {
  ABI abi;
  bool is64Bit;           // Whether ABI uses 64-bit registers
  bool has64BitPointers;  // Whether ABI uses 64-bit pointers
  int numArgRegs;         // Number of integer argument registers
  int numFPArgRegs;       // Number of FP argument registers
  int stackAlignment;     // Stack alignment in bytes
  const char *name;       // ABI name string
};

// Get ABI information
inline ABIInfo GetABIInfo(ABI abi) {
  switch (abi) {
  case ABI::O32:
    return {ABI::O32, false, false, 4, 2, 8, "o32"};
  case ABI::N32:
    return {ABI::N32, true, false, 8, 8, 16, "n32"};
  case ABI::N64:
    return {ABI::N64, true, true, 8, 8, 16, "n64"};
  case ABI::O64:
    return {ABI::O64, true, true, 4, 2, 16, "o64"};
  case ABI::EABI:
    return {ABI::EABI, false, false, 8, 8, 8, "eabi"};
  case ABI::NUBI:
    return {ABI::NUBI, true, true, 8, 8, 16, "nubi"};
  default:
    return {ABI::Unknown, false, false, 0, 0, 0, "unknown"};
  }
}

// Detect ABI from compiler flags
inline ABI DetectABI() {
#if defined(_ABIO32)
  return ABI::O32;
#elif defined(_ABIN32)
  return ABI::N32;
#elif defined(_ABI64)
  return ABI::N64;
#elif defined(_ABIO64)
  return ABI::O64;
#elif defined(__mips_eabi)
  return ABI::EABI;
#elif defined(__mips_nubi)
  return ABI::NUBI;
#else
  // Fallback heuristics
#if defined(__mips64) || defined(__mips64__)
  // MIPS64 defaults to N64
  return ABI::N64;
#else
  // MIPS32 defaults to O32
  return ABI::O32;
#endif
#endif
}

// Get the name of an ABI
inline const char *GetABIName(ABI abi) {
  return GetABIInfo(abi).name;
}

} // namespace MIPS
} // namespace Architecture
} // namespace ds2
