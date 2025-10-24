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

namespace ds2 {
namespace Host {
namespace Linux {
namespace MIPS {

// MIPS Hardware Capabilities (from Linux kernel asm/hwcap.h)
// These flags indicate which ISA extensions are available
enum /* HwCaps */ {
  // Basic ISA features
  HWCAP_MIPS_R6 = (1 << 0),         // MIPS Release 6
  HWCAP_MIPS_MSA = (1 << 1),        // MIPS SIMD Architecture

  // Code compression
  HWCAP_MIPS16 = (1 << 2),          // MIPS16e ASE
  HWCAP_MICROMIPS = (1 << 3),       // microMIPS ASE

  // Floating point
  HWCAP_MIPS_FPU = (1 << 4),        // Hardware FPU present
  HWCAP_MIPS_PAIRED_SINGLE = (1 << 5), // Paired-single instructions

  // SIMD and multimedia extensions
  HWCAP_MIPS_MDMX = (1 << 6),       // MIPS Digital Media Extension (MDMX)
  HWCAP_MIPS_3D = (1 << 7),         // MIPS-3D ASE

  // DSP extensions
  HWCAP_MIPS_DSP = (1 << 8),        // DSP ASE
  HWCAP_MIPS_DSP2 = (1 << 9),       // DSP ASE Revision 2
  HWCAP_MIPS_DSP3 = (1 << 10),      // DSP ASE Revision 3

  // SmartMIPS
  HWCAP_MIPS_SMARTMIPS = (1 << 11), // SmartMIPS ASE

  // Multi-threading
  HWCAP_MIPS_MT = (1 << 12),        // Multi-Threading ASE
  HWCAP_MIPS_VZ = (1 << 13),        // Virtualization ASE

  // Memory management extensions
  HWCAP_MIPS_EVA = (1 << 14),       // Enhanced Virtual Addressing
  HWCAP_MIPS_LDPTE = (1 << 15),     // Load-linked/Store-conditional page table entry

  // Performance extensions
  HWCAP_MIPS_PERFCOUNT = (1 << 16), // Performance counters
  HWCAP_MIPS_WATCH = (1 << 17),     // Watchpoint registers

  // Additional features
  HWCAP_MIPS_LLSC = (1 << 18),      // Load-linked/Store-conditional
  HWCAP_MIPS_SYNC = (1 << 19),      // Sync instruction support
  HWCAP_MIPS_CACHE = (1 << 20),     // Cache instruction support

  // 64-bit extensions
  HWCAP_MIPS_64BIT = (1 << 21),     // 64-bit registers and operations
  HWCAP_MIPS_MIPS64R2 = (1 << 22),  // MIPS64 Release 2 or higher

  // Branch optimization
  HWCAP_MIPS_BRANCH_LIKELY = (1 << 23), // Branch-likely instructions

  // CRC32
  HWCAP_MIPS_CRC32 = (1 << 24),     // CRC32 instructions

  // GINV (Global Invalidate)
  HWCAP_MIPS_GINV = (1 << 25),      // Global Invalidate instruction
};

// Extension compatibility matrix
inline bool IsMicroMIPSCompatible(unsigned long hwcaps) {
  return (hwcaps & HWCAP_MICROMIPS) != 0;
}

inline bool IsMIPS16Compatible(unsigned long hwcaps) {
  return (hwcaps & HWCAP_MIPS16) != 0;
}

inline bool HasDSPSupport(unsigned long hwcaps) {
  return (hwcaps & (HWCAP_MIPS_DSP | HWCAP_MIPS_DSP2 | HWCAP_MIPS_DSP3)) != 0;
}

inline bool HasSIMDSupport(unsigned long hwcaps) {
  return (hwcaps & (HWCAP_MIPS_MSA | HWCAP_MIPS_MDMX)) != 0;
}

inline bool HasMT(unsigned long hwcaps) {
  return (hwcaps & HWCAP_MIPS_MT) != 0;
}

inline bool HasVirtualization(unsigned long hwcaps) {
  return (hwcaps & HWCAP_MIPS_VZ) != 0;
}
} // namespace MIPS
} // namespace Linux
} // namespace Host
} // namespace ds2
