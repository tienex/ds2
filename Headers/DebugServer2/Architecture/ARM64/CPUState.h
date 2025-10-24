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

#if !defined(CPUSTATE_H_INTERNAL)
#error "You shall not include this file directly."
#endif

#include "DebugServer2/Architecture/ARM/CPUState.h" // Include the A32 variant
#include "DebugServer2/Architecture/ARM64/RegistersDescriptors.h"

namespace ds2 {
namespace Architecture {
namespace ARM64 {

//
// VFP is shared between A32 and A64
//
using ds2::Architecture::ARM::VFPDouble;
using ds2::Architecture::ARM::VFPQuad;
using ds2::Architecture::ARM::VFPSingle;

//
// Import the A32 variant
//
typedef ds2::Architecture::ARM::CPUState CPUState32;

//
// Define the 64-bit variant
//
struct CPUState64 {
  union {
    uint64_t regs[31 + 1 + 1 + 1];
    struct {
      uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
          x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28,
          fp, lr, sp, pc, cpsr;
    };
  } gp;

  // Advanced SIMD (NEON) and Floating-Point registers
  // V0-V31 (128-bit), which can be accessed as:
  // - B0-B31 (8-bit)
  // - H0-H31 (16-bit, half-precision)
  // - S0-S31 (32-bit, single-precision)
  // - D0-D31 (64-bit, double-precision)
  // - Q0-Q31 (128-bit, quad-word)
  struct {
    union {
      uint8_t b[32][16];       // 8-bit view
      uint16_t h[32][8];       // 16-bit view (half-precision FP)
      VFPSingle sng[32];       // 32-bit view (aliases S0-S31)
      VFPDouble dbl[32];       // 64-bit view (aliases D0-D31)
      VFPQuad quad[32];        // 128-bit view (Q0-Q31, full V registers)
      __uint128_t v[32];       // 128-bit raw view
    };
    uint32_t fpsr;             // Floating-Point Status Register
    uint32_t fpcr;             // Floating-Point Control Register
  } vfp;

  // SVE (Scalable Vector Extension) - ARMv8.2-A and later
  // Vector length can be 128 to 2048 bits
  struct {
    // Z registers (Z0-Z31) - scalable vector registers
    // Maximum 2048 bits (256 bytes) per register
    uint8_t z[32][256];        // Z0-Z31, up to 2048 bits each

    // P registers (P0-P15) - predicate registers
    // Maximum 256 bits (32 bytes) per register (one bit per byte in Z)
    uint8_t p[16][32];         // P0-P15 predicate registers

    // FFR - First Fault Register (also a predicate register)
    uint8_t ffr[32];           // First Fault Register

    uint32_t vl;               // Vector Length (in bytes)
    uint32_t  reserved;
  } sve;

  // SME (Scalable Matrix Extension) - ARMv9.2-A
  struct {
    // ZA array storage - square matrix of SVE vector length
    // Maximum 2048x2048 bits
    uint8_t za[256][256];      // ZA array (up to 256x256 bytes)

    // Streaming SVE mode uses separate ZT0 register (SME2)
    uint8_t zt0[64];           // ZT0 register (512 bits for SME2)

    uint64_t svcr;             // Streaming Vector Control Register
    uint64_t smcr_el1;         // SME Control Register (EL1)
    uint32_t svl;              // Streaming Vector Length (in bytes)
    uint32_t reserved;
  } sme;

  // MTE (Memory Tagging Extension) - ARMv8.5-A
  struct {
    uint64_t gcr_el1;          // Tag Control Register (EL1)
    uint64_t rgsr_el1;         // Random Allocation Tag Seed Register
    uint64_t tfsr_el1;         // Tag Fault Status Register (EL1)
    uint64_t tfsre0_el1;       // Tag Fault Status Register (EL0)
  } mte;

  // Pointer Authentication - ARMv8.3-A
  struct {
    uint64_t apiakey[2];       // APIAKey_EL1 (128-bit)
    uint64_t apibkey[2];       // APIBKey_EL1 (128-bit)
    uint64_t apdakey[2];       // APDAKey_EL1 (128-bit)
    uint64_t apdbkey[2];       // APDBKey_EL1 (128-bit)
    uint64_t apgakey[2];       // APGAKey_EL1 (128-bit)
  } pac;

  // System registers
  struct {
    uint64_t tpidr_el0;        // Thread ID Register (EL0)
    uint64_t tpidrro_el0;      // Thread ID Register, Read-Only (EL0)
    uint64_t tpidr_el1;        // Thread ID Register (EL1)

    // Cache information
    uint64_t ctr_el0;          // Cache Type Register

    // Multiprocessor affinity
    uint64_t mpidr_el1;        // Multiprocessor Affinity Register

    // Feature registers
    uint64_t id_aa64pfr0_el1;  // Processor Feature Register 0
    uint64_t id_aa64pfr1_el1;  // Processor Feature Register 1
    uint64_t id_aa64dfr0_el1;  // Debug Feature Register 0
    uint64_t id_aa64dfr1_el1;  // Debug Feature Register 1
    uint64_t id_aa64isar0_el1; // Instruction Set Attribute Register 0
    uint64_t id_aa64isar1_el1; // Instruction Set Attribute Register 1
    uint64_t id_aa64isar2_el1; // Instruction Set Attribute Register 2
    uint64_t id_aa64mmfr0_el1; // Memory Model Feature Register 0
    uint64_t id_aa64mmfr1_el1; // Memory Model Feature Register 1
    uint64_t id_aa64mmfr2_el1; // Memory Model Feature Register 2
  } system;

  // Generic Timer
  struct {
    uint64_t cntvct_el0;       // Virtual Count Register
    uint64_t cntfrq_el0;       // Counter Frequency Register
    uint64_t cntkctl_el1;      // Timer Control Register (EL1)
    uint64_t cntp_ctl_el0;     // Physical Timer Control Register
    uint64_t cntp_cval_el0;    // Physical Timer CompareValue Register
    uint64_t cntv_ctl_el0;     // Virtual Timer Control Register
    uint64_t cntv_cval_el0;    // Virtual Timer CompareValue Register
  } timer;

  // Performance Monitors Extension
  struct {
    uint64_t pmcr_el0;         // Performance Monitors Control Register
    uint64_t pmcntenset_el0;   // Count Enable Set Register
    uint64_t pmcntenclr_el0;   // Count Enable Clear Register
    uint64_t pmovsclr_el0;     // Overflow Flag Status Clear Register
    uint64_t pmswinc_el0;      // Software Increment Register
    uint64_t pmselr_el0;       // Event Counter Selection Register
    uint64_t pmceid0_el0;      // Common Event Identification Register 0
    uint64_t pmceid1_el0;      // Common Event Identification Register 1
    uint64_t pmccntr_el0;      // Cycle Count Register
    uint64_t pmxevtyper_el0;   // Event Type Register
    uint64_t pmxevcntr_el0;    // Event Count Register
    uint64_t pmuserenr_el0;    // User Enable Register
    uint64_t pmintenset_el1;   // Interrupt Enable Set Register
    uint64_t pmintenclr_el1;   // Interrupt Enable Clear Register
  } pmu;

  //
  // Accessors
  //
  inline uint64_t pc() const { return gp.pc; }
  inline void setPC(uint64_t pc) { gp.pc = pc; }

  inline uint64_t sp() const { return gp.sp; }
  inline void setSP(uint64_t sp) { gp.sp = sp; }

  inline uint64_t retval() const { return gp.x0; }

public:
  inline void getGPState(GPRegisterValueVector &regs) const {
    regs.clear();
    for (size_t n = 0; n < array_sizeof(gp.regs); n++) {
      regs.push_back(GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]});
    }
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    for (size_t n = 0; n < regs.size() && n < array_sizeof(gp.regs); n++) {
      gp.regs[n] = regs[n];
    }
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (forLLDB) {
      for (size_t n = 0; n < 33; n++) {
        regs[n + reg_lldb_x0] = GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
      }
      regs[reg_lldb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    } else {
      // GDB can live with non-zero registers
      for (size_t n = 0; n < 33; n++) {
        if (n >= 13) {
          regs[n + reg_gdb_x0] =
              GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
        }
      }
      regs[reg_gdb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_lldb_x0 && regno <= reg_lldb_x30) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_lldb_x0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_lldb_sp) {
      *ptr = const_cast<uint64_t *>(&gp.sp);
      *length = sizeof(gp.sp);
    } else if (regno == reg_lldb_pc) {
      *ptr = const_cast<uint64_t *>(&gp.pc);
      *length = sizeof(gp.pc);
    } else if (regno == reg_lldb_cpsr) {
      *ptr = const_cast<uint64_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else {
      return false;
    }

    return true;
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_gdb_x0 && regno <= reg_gdb_x30) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_gdb_x0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_gdb_sp) {
      *ptr = const_cast<uint64_t *>(&gp.sp);
      *length = sizeof(gp.sp);
    } else if (regno == reg_gdb_pc) {
      *ptr = const_cast<uint64_t *>(&gp.pc);
      *length = sizeof(gp.pc);
    } else if (regno == reg_gdb_cpsr) {
      *ptr = const_cast<uint64_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else {
      return false;
    }

    return true;
  }
};

//
// Define the union of the two variants, this is the public
// structure.
//

struct CPUState {
  bool isA32; // Select which is valid between state32 and state64.

  union {
    CPUState32 state32;
    CPUState64 state64;
  };

  CPUState() : state64() {}

  //
  // Accessors
  //
  inline uint64_t pc() const {
    return isA32 ? static_cast<uint64_t>(state32.pc()) : state64.pc();
  }
  inline void setPC(uint64_t pc) {
    if (isA32)
      state32.setPC(pc);
    else
      state64.setPC(pc);
  }

  inline uint64_t sp() const {
    return isA32 ? static_cast<uint64_t>(state32.sp()) : state64.sp();
  }
  inline void setSP(uint64_t sp) {
    if (isA32)
      state32.setSP(sp);
    else
      state64.setSP(sp);
  }

  inline uint64_t retval() const {
    return isA32 ? static_cast<uint64_t>(state32.retval()) : state64.retval();
  }

  inline bool isThumb() const { return isA32 ? state32.isThumb() : false; }

public:
  inline void getGPState(GPRegisterValueVector &regs) const {
    if (isA32) {
      state32.getGPState(regs);
    } else {
      state64.getGPState(regs);
    }
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    if (isA32) {
      state32.setGPState(regs);
    } else {
      state64.setGPState(regs);
    }
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (isA32) {
      state32.getStopGPState(regs, forLLDB);
    } else {
      state64.getStopGPState(regs, forLLDB);
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (isA32) {
      return state32.getLLDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getLLDBRegisterPtr(regno, ptr, length);
    }
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (isA32) {
      return state32.getGDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getGDBRegisterPtr(regno, ptr, length);
    }
  }
};
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
