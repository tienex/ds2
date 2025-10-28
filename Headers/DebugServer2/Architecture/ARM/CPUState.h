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

#include "DebugServer2/Architecture/ARM/RegistersDescriptors.h"

#include <cstring>

namespace ds2 {
namespace Architecture {
namespace ARM {

struct VFPSingle {
#ifdef ENDIAN_BIG
  uint32_t : 32;
  uint32_t value;
#else
  uint32_t value;
  uint32_t : 32;
#endif
};

struct VFPDouble {
  uint64_t value;
};

struct VFPQuad {
#ifdef ENDIAN_BIG
  uint64_t hi;
  uint64_t lo;
#else
  uint64_t lo;
  uint64_t hi;
#endif
};

#pragma pack(push, 1)

struct CPUState {
  union {
    uint32_t regs[16 + 1];
    struct {
      uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, sp, lr, pc,
          cpsr;
    };
  } gp;

  struct {
    union {
      VFPSingle sng[32];   // S0-S31 (VFPv2+)
      VFPDouble dbl[32];   // D0-D31 (D0-D15 VFPv2, D16-D31 VFPv3+)
      VFPQuad quad[16];    // Q0-Q15 (NEON/Advanced SIMD)
      uint64_t d[32];      // Alias for D registers as raw uint64
    };
    uint32_t fpscr;        // Floating-Point Status and Control Register
    uint32_t fpexc;        // Floating-Point Exception Register (VFPv2+)
    uint32_t fpsid;        // Floating-Point System ID Register (VFPv2+)
  } vfp;

  // NEON/Advanced SIMD specific state (ARMv7-A with NEON)
  struct {
    // NEON shares register file with VFP (Q0-Q15 = D0-D31)
    // Additional control register
    uint32_t mvfr0;        // Media and VFP Feature Register 0
    uint32_t mvfr1;        // Media and VFP Feature Register 1
    uint32_t mvfr2;        // Media and VFP Feature Register 2 (ARMv8-A)
  } neon;

  struct {
    // Breakpoints
    uint32_t bp_ctrl[32];
    uint32_t bp_addr[32];

    // Watchpoints
    uint32_t wp_ctrl[32];
    uint32_t wp_addr[32];
  } hbp;

  // System control registers (CP15)
  struct {
    uint32_t sctlr;        // System Control Register
    uint32_t actlr;        // Auxiliary Control Register
    uint32_t cpacr;        // Coprocessor Access Control Register

    // Thread ID registers (for TLS support)
    uint32_t tpidruro;     // User Read-Only Thread ID Register
    uint32_t tpidrurw;     // User Read/Write Thread ID Register
    uint32_t tpidrprw;     // PL1 only Thread ID Register

    // Cache Type Register
    uint32_t ctr;          // Cache Type Register

    // Multiprocessor Affinity Register
    uint32_t mpidr;        // Multiprocessor Affinity Register
  } system;

  // Generic Timer (ARMv7-A Generic Timer Extension)
  struct {
    uint64_t cntvct;       // Virtual Count register
    uint64_t cntfrq;       // Counter Frequency register
    uint32_t cntkctl;      // Timer PL1 Control register
    uint32_t cntp_ctl;     // Physical Timer Control register
    uint32_t cntp_cval;    // Physical Timer CompareValue register
    uint32_t cntv_ctl;     // Virtual Timer Control register
    uint32_t cntv_cval;    // Virtual Timer CompareValue register
  } timer;

  // Performance Monitors Extension (ARMv7-A)
  struct {
    uint32_t pmcr;         // Performance Monitors Control Register
    uint32_t pmcntenset;   // Count Enable Set register
    uint32_t pmcntenclr;   // Count Enable Clear register
    uint32_t pmovsr;       // Overflow Flag Status Register
    uint32_t pmswinc;      // Software Increment register
    uint32_t pmselr;       // Event Counter Selection Register
    uint32_t pmceid0;      // Common Event Identification register 0
    uint32_t pmceid1;      // Common Event Identification register 1
    uint32_t pmccntr;      // Cycle Count Register (64-bit)
    uint32_t pmxevtyper;   // Event Type Select Register
    uint32_t pmxevcntr;    // Event Count Register
    uint32_t pmuserenr;    // User Enable Register
    uint32_t pmintenset;   // Interrupt Enable Set register
    uint32_t pmintenclr;   // Interrupt Enable Clear register
  } pmu;

public:
  CPUState() { clear(); }

  inline void clear() {
    std::memset(&gp, 0, sizeof(gp));
    std::memset(&vfp, 0, sizeof(vfp));
    std::memset(&neon, 0, sizeof(neon));
    std::memset(&hbp, 0, sizeof(hbp));
    std::memset(&system, 0, sizeof(system));
    std::memset(&timer, 0, sizeof(timer));
    std::memset(&pmu, 0, sizeof(pmu));
  }

public:
  //
  // Accessors
  //
  inline uint32_t pc() const { return gp.pc; }
  inline void setPC(uint32_t pc) { gp.pc = pc; }

  //
  // xpc returns the thumb bit
  //
  inline uint32_t xpc() const { return gp.pc | (isThumb() ? 1 : 0); }

  inline uint32_t sp() const { return gp.sp; }
  inline void setSP(uint32_t sp) { gp.sp = sp; }

  inline uint32_t retval() const { return gp.r0; }

  inline bool isThumb() const { return (gp.cpsr & (1 << 5)) != 0; }

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
      for (size_t n = 0; n < 16; n++) {
        regs[n + reg_lldb_r0] = GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
      }
      regs[reg_lldb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    } else {
      //
      // GDB can live with non-zero registers
      //
      for (size_t n = 0; n < 16; n++) {
        if (n >= 13) {
          regs[n + reg_gdb_r0] =
              GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
        }
      }
      regs[reg_gdb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_lldb_r0 && regno <= reg_lldb_r15) {
      *ptr = const_cast<uint32_t *>(&gp.regs[regno - reg_lldb_r0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_lldb_cpsr) {
      *ptr = const_cast<uint32_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else if (regno >= reg_lldb_d0 && regno <= reg_lldb_d31) {
      *ptr = const_cast<VFPDouble *>(&vfp.dbl[regno - reg_lldb_d0]);
      *length = sizeof(vfp.dbl[0]);
    } else if (regno >= reg_lldb_s0 && regno <= reg_lldb_s31) {
      *ptr = const_cast<uint32_t *>(&vfp.sng[regno - reg_lldb_s0].value);
      *length = sizeof(vfp.sng[0].value);
    } else if (regno >= reg_lldb_q0 && regno <= reg_lldb_q15) {
      *ptr = const_cast<VFPQuad *>(&vfp.quad[regno - reg_lldb_q0]);
      *length = sizeof(vfp.quad[0]);
    } else if (regno == reg_lldb_fpscr) {
      *ptr = const_cast<uint32_t *>(&vfp.fpscr);
      *length = sizeof(vfp.fpscr);
    } else {
      return false;
    }

    return true;
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_gdb_r0 && regno <= reg_gdb_r15) {
      *ptr = const_cast<uint32_t *>(&gp.regs[regno - reg_gdb_r0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_gdb_cpsr) {
      *ptr = const_cast<uint32_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else if (regno >= reg_gdb_d0 && regno <= reg_gdb_d31) {
      *ptr = const_cast<VFPDouble *>(&vfp.dbl[regno - reg_gdb_d0]);
      *length = sizeof(vfp.dbl[0]);
    } else if (regno == reg_gdb_fpscr) {
      *ptr = const_cast<uint32_t *>(&vfp.fpscr);
      *length = sizeof(vfp.fpscr);
    } else {
      return false;
    }

    return true;
  }
};

#pragma pack(pop)
} // namespace ARM
} // namespace Architecture
} // namespace ds2
