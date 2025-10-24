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

#include "DebugServer2/Architecture/MIPS64/RegistersDescriptors.h"

#include <cstring>

namespace ds2 {
namespace Architecture {
namespace MIPS64 {

#pragma pack(push, 1)

struct CPUState {
  union {
    uint64_t regs[32];
    struct {
      uint64_t zero, at, v0, v1, a0, a1, a2, a3;
      uint64_t t0, t1, t2, t3, t4, t5, t6, t7;
      uint64_t s0, s1, s2, s3, s4, s5, s6, s7;
      uint64_t t8, t9, k0, k1, gp, sp, s8, ra;
    };
  } gp;

  struct {
    uint64_t lo;
    uint64_t hi;
    uint64_t pc;
  } special;

  struct {
    uint64_t status;   // CP0 Status register
    uint64_t badvaddr; // CP0 BadVAddr register
    uint64_t cause;    // CP0 Cause register
  } cop0;

  struct {
    uint64_t fpr[32]; // 64-bit floating-point registers
    uint32_t fcsr;    // Floating-point Control/Status Register
    uint32_t fir;     // Floating-point Implementation Register
  } fpu;

  // DSP ASE registers
  struct {
    uint64_t ac[4];  // DSP accumulators ac0-ac3
    uint32_t dspctl; // DSP control register
  } dsp;

  // MSA (MIPS SIMD Architecture) registers
  struct {
    uint8_t w[32][16]; // 32 128-bit vector registers
    uint32_t csr;      // MSA Control and Status Register
    uint32_t ir;       // MSA Implementation Register
  } msa;

public:
  CPUState() { clear(); }

  inline void clear() {
    std::memset(&gp, 0, sizeof(gp));
    std::memset(&special, 0, sizeof(special));
    std::memset(&cop0, 0, sizeof(cop0));
    std::memset(&fpu, 0, sizeof(fpu));
    std::memset(&dsp, 0, sizeof(dsp));
    std::memset(&msa, 0, sizeof(msa));
  }

public:
  //
  // Accessors
  //
  inline uint64_t pc() const { return special.pc; }
  inline void setPC(uint64_t pc) { special.pc = pc; }

  //
  // xpc returns the ISA mode bit (microMIPS/MIPS16)
  //
  inline uint64_t xpc() const { return special.pc; }

  inline uint64_t sp() const { return gp.sp; }
  inline void setSP(uint64_t sp) { gp.sp = sp; }

  inline uint64_t retval() const { return gp.v0; }

  // Check if in compressed mode (microMIPS or MIPS16)
  inline bool isCompressedMode() const { return (special.pc & 0x1) != 0; }

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
      for (size_t n = 0; n < 32; n++) {
        regs[n + reg_lldb_r0] = GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
      }
      regs[reg_lldb_lo] = GPRegisterValue{sizeof(special.lo), special.lo};
      regs[reg_lldb_hi] = GPRegisterValue{sizeof(special.hi), special.hi};
      regs[reg_lldb_pc] = GPRegisterValue{sizeof(special.pc), special.pc};
      regs[reg_lldb_status] = GPRegisterValue{sizeof(cop0.status), cop0.status};
      regs[reg_lldb_badvaddr] = GPRegisterValue{sizeof(cop0.badvaddr), cop0.badvaddr};
      regs[reg_lldb_cause] = GPRegisterValue{sizeof(cop0.cause), cop0.cause};
    } else {
      // GDB registers
      for (size_t n = 0; n < 32; n++) {
        if (n >= 28) { // sp, s8, ra
          regs[n + reg_gdb_r0] = GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
        }
      }
      regs[reg_gdb_status] = GPRegisterValue{sizeof(cop0.status), cop0.status};
      regs[reg_gdb_lo] = GPRegisterValue{sizeof(special.lo), special.lo};
      regs[reg_gdb_hi] = GPRegisterValue{sizeof(special.hi), special.hi};
      regs[reg_gdb_badvaddr] = GPRegisterValue{sizeof(cop0.badvaddr), cop0.badvaddr};
      regs[reg_gdb_cause] = GPRegisterValue{sizeof(cop0.cause), cop0.cause};
      regs[reg_gdb_pc] = GPRegisterValue{sizeof(special.pc), special.pc};
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_lldb_r0 && regno <= reg_lldb_r31) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_lldb_r0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_lldb_lo) {
      *ptr = const_cast<uint64_t *>(&special.lo);
      *length = sizeof(special.lo);
    } else if (regno == reg_lldb_hi) {
      *ptr = const_cast<uint64_t *>(&special.hi);
      *length = sizeof(special.hi);
    } else if (regno == reg_lldb_pc) {
      *ptr = const_cast<uint64_t *>(&special.pc);
      *length = sizeof(special.pc);
    } else if (regno == reg_lldb_status) {
      *ptr = const_cast<uint64_t *>(&cop0.status);
      *length = sizeof(cop0.status);
    } else if (regno == reg_lldb_badvaddr) {
      *ptr = const_cast<uint64_t *>(&cop0.badvaddr);
      *length = sizeof(cop0.badvaddr);
    } else if (regno == reg_lldb_cause) {
      *ptr = const_cast<uint64_t *>(&cop0.cause);
      *length = sizeof(cop0.cause);
    } else if (regno >= reg_lldb_f0 && regno <= reg_lldb_f31) {
      *ptr = const_cast<uint64_t *>(&fpu.fpr[regno - reg_lldb_f0]);
      *length = sizeof(fpu.fpr[0]);
    } else if (regno == reg_lldb_fcsr) {
      *ptr = const_cast<uint32_t *>(&fpu.fcsr);
      *length = sizeof(fpu.fcsr);
    } else if (regno == reg_lldb_fir) {
      *ptr = const_cast<uint32_t *>(&fpu.fir);
      *length = sizeof(fpu.fir);
    } else {
      return false;
    }

    return true;
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_gdb_r0 && regno <= reg_gdb_r31) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_gdb_r0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_gdb_status) {
      *ptr = const_cast<uint64_t *>(&cop0.status);
      *length = sizeof(cop0.status);
    } else if (regno == reg_gdb_lo) {
      *ptr = const_cast<uint64_t *>(&special.lo);
      *length = sizeof(special.lo);
    } else if (regno == reg_gdb_hi) {
      *ptr = const_cast<uint64_t *>(&special.hi);
      *length = sizeof(special.hi);
    } else if (regno == reg_gdb_badvaddr) {
      *ptr = const_cast<uint64_t *>(&cop0.badvaddr);
      *length = sizeof(cop0.badvaddr);
    } else if (regno == reg_gdb_cause) {
      *ptr = const_cast<uint64_t *>(&cop0.cause);
      *length = sizeof(cop0.cause);
    } else if (regno == reg_gdb_pc) {
      *ptr = const_cast<uint64_t *>(&special.pc);
      *length = sizeof(special.pc);
    } else if (regno >= reg_gdb_f0 && regno <= reg_gdb_f31) {
      *ptr = const_cast<uint64_t *>(&fpu.fpr[regno - reg_gdb_f0]);
      *length = sizeof(fpu.fpr[0]);
    } else if (regno == reg_gdb_fcsr) {
      *ptr = const_cast<uint32_t *>(&fpu.fcsr);
      *length = sizeof(fpu.fcsr);
    } else if (regno == reg_gdb_fir) {
      *ptr = const_cast<uint32_t *>(&fpu.fir);
      *length = sizeof(fpu.fir);
    } else {
      return false;
    }

    return true;
  }
};

#pragma pack(pop)
} // namespace MIPS64
} // namespace Architecture
} // namespace ds2
