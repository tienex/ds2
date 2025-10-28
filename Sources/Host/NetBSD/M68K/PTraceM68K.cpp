//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/NetBSD/PTrace.h"
#include "DebugServer2/Architecture/M68K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::NetBSD::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace NetBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  //
  // Read general-purpose registers
  // NetBSD m68k struct reg layout
  //
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy m68k registers
  // NetBSD struct reg for m68k contains:
  // - r_regs[16]: d0-d7, a0-a7
  // - r_sr: status register
  // - r_pc: program counter
  //
  for (int i = 0; i < 8; i++) {
    state.data.regs[i] = gprs.r_regs[i];
  }

  for (int i = 0; i < 7; i++) {
    state.addr.regs[i] = gprs.r_regs[8 + i];
  }

  //
  // a7 is the stack pointer (usp or ssp depending on mode)
  //
  if (gprs.r_sr & 0x2000) {  // Supervisor mode
    state.stack.ssp = gprs.r_regs[15];
  } else {
    state.stack.usp = gprs.r_regs[15];
  }

  state.special.pc = gprs.r_pc;
  state.special.sr = gprs.r_sr;

  //
  // Copy SR bit fields
  //
  std::memcpy(&state.sr_flags, &gprs.r_sr, sizeof(state.sr_flags));

  //
  // Read floating-point registers
  // NetBSD m68k FPU support
  //
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fpregs, 0) >= 0) {
    //
    // m68k FPU: 8 extended-precision (80-bit) registers
    //
    for (int i = 0; i < 8; i++) {
      state.fpu.fpr[i].mantissa = fpregs.r_fpregs[i].mantissa;
      state.fpu.fpr[i].exponent = fpregs.r_fpregs[i].exponent;
    }

    // FPU control registers
    state.fpu.fpcr = fpregs.r_fpcr;
    state.fpu.fpsr = fpregs.r_fpsr;
    state.fpu.fpiar = fpregs.r_fpiar;
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
  struct reg gprs;

  for (int i = 0; i < 8; i++) {
    gprs.r_regs[i] = state.data.regs[i];
  }

  for (int i = 0; i < 7; i++) {
    gprs.r_regs[8 + i] = state.addr.regs[i];
  }

  //
  // Set stack pointer based on mode
  //
  if (state.special.sr & 0x2000) {  // Supervisor mode
    gprs.r_regs[15] = state.stack.ssp;
  } else {
    gprs.r_regs[15] = state.stack.usp;
  }

  gprs.r_pc = state.special.pc;
  gprs.r_sr = state.special.sr;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;

  for (int i = 0; i < 8; i++) {
    fpregs.r_fpregs[i].mantissa = state.fpu.fpr[i].mantissa;
    fpregs.r_fpregs[i].exponent = state.fpu.fpr[i].exponent;
  }

  fpregs.r_fpcr = state.fpu.fpcr;
  fpregs.r_fpsr = state.fpu.fpsr;
  fpregs.r_fpiar = state.fpu.fpiar;

  if (wrapPtrace(PT_SETFPREGS, pid, &fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
