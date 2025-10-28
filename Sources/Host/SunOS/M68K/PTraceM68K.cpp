//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SunOS/PTrace.h"
#include "DebugServer2/Architecture/M68K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::SunOS::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace SunOS {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // SunOS m68k support (Sun-2, Sun-3)
  // SunOS 3.x, 4.x for 68010, 68020, 68030
  // Uses ptrace interface
  //
  pid_t pid = ptid.pid;

  //
  // Read general-purpose registers
  // SunOS m68k struct regs layout
  //
  struct regs {
    int r_dreg[8];       // d0-d7
    int r_areg[8];       // a0-a7
    int r_pc;            // program counter
    int r_sr;            // status register
  } gprs;

  if (::ptrace(PTRACE_GETREGS, pid, (char *)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy data registers
  //
  for (int i = 0; i < 8; i++) {
    state.data.regs[i] = gprs.r_dreg[i];
  }

  //
  // Copy address registers (a0-a6)
  //
  for (int i = 0; i < 7; i++) {
    state.addr.regs[i] = gprs.r_areg[i];
  }

  //
  // a7 is the stack pointer
  // SunOS stores it based on current mode
  //
  if (gprs.r_sr & 0x2000) {  // Supervisor mode
    state.stack.ssp = gprs.r_areg[7];
  } else {
    state.stack.usp = gprs.r_areg[7];
  }

  state.special.pc = gprs.r_pc;
  state.special.sr = gprs.r_sr;

  //
  // Copy SR bit fields
  //
  std::memcpy(&state.sr_flags, &gprs.r_sr, sizeof(state.sr_flags));

  //
  // Read floating-point registers
  // SunOS m68k FPU support (68881/68882)
  //
#if defined(PTRACE_GETFPREGS)
  struct fpregs {
    int f_pcr;           // FP control register
    int f_psr;           // FP status register
    int f_piaddr;        // FP instruction address
    struct {
      int exp;
      int mantissa[2];
    } f_fpregs[8];
  } fpregs;

  if (::ptrace(PTRACE_GETFPREGS, pid, (char *)&fpregs, 0) >= 0) {
    //
    // Copy FPU registers (80-bit extended precision)
    //
    for (int i = 0; i < 8; i++) {
      state.fpu.fpr[i].mantissa = ((uint64_t)fpregs.f_fpregs[i].mantissa[0] << 32) |
                                  fpregs.f_fpregs[i].mantissa[1];
      state.fpu.fpr[i].exponent = fpregs.f_fpregs[i].exp & 0xFFFF;
    }

    // FPU control registers
    state.fpu.fpcr = fpregs.f_pcr;
    state.fpu.fpsr = fpregs.f_psr;
    state.fpu.fpiar = fpregs.f_piaddr;
  }
#endif

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;

  //
  // Write general-purpose registers
  //
  struct regs {
    int r_dreg[8];
    int r_areg[8];
    int r_pc;
    int r_sr;
  } gprs;

  for (int i = 0; i < 8; i++) {
    gprs.r_dreg[i] = state.data.regs[i];
  }

  for (int i = 0; i < 7; i++) {
    gprs.r_areg[i] = state.addr.regs[i];
  }

  //
  // Set stack pointer based on mode
  //
  if (state.special.sr & 0x2000) {  // Supervisor mode
    gprs.r_areg[7] = state.stack.ssp;
  } else {
    gprs.r_areg[7] = state.stack.usp;
  }

  gprs.r_pc = state.special.pc;
  gprs.r_sr = state.special.sr;

  if (::ptrace(PTRACE_SETREGS, pid, (char *)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
#if defined(PTRACE_SETFPREGS)
  struct fpregs {
    int f_pcr;
    int f_psr;
    int f_piaddr;
    struct {
      int exp;
      int mantissa[2];
    } f_fpregs[8];
  } fpregs;

  for (int i = 0; i < 8; i++) {
    fpregs.f_fpregs[i].mantissa[0] = state.fpu.fpr[i].mantissa >> 32;
    fpregs.f_fpregs[i].mantissa[1] = state.fpu.fpr[i].mantissa & 0xFFFFFFFF;
    fpregs.f_fpregs[i].exp = state.fpu.fpr[i].exponent;
  }

  fpregs.f_pcr = state.fpu.fpcr;
  fpregs.f_psr = state.fpu.fpsr;
  fpregs.f_piaddr = state.fpu.fpiar;

  if (::ptrace(PTRACE_SETFPREGS, pid, (char *)&fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace SunOS
} // namespace Host
} // namespace ds2
