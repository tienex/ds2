//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/OpenBSD/PTrace.h"
#include "DebugServer2/Architecture/MIPS64/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::OpenBSD::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace OpenBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  //
  // Read general-purpose registers
  //
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy MIPS64 general purpose registers (64-bit)
  // OpenBSD MIPS64 struct reg layout
  //
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gprs.r_regs[i];
  }
  state.special.lo = gprs.r_mullo;
  state.special.hi = gprs.r_mulhi;
  state.special.pc = gprs.r_pc;

  //
  // Copy COP0 status registers
  //
  state.cop0.status = gprs.r_sr;
  state.cop0.badvaddr = gprs.r_badvaddr;
  state.cop0.cause = gprs.r_cause;

  //
  // Read FPU registers if available
  //
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fpregs, 0) >= 0) {
    // MIPS64 uses 32 double-precision (64-bit) FPU registers
    for (int i = 0; i < 32; i++) {
      state.fpu.fpr[i] = fpregs.r_regs[i];
    }
    state.fpu.fcsr = fpregs.r_fcsr;
    state.fpu.fir = fpregs.r_fir;
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
  for (int i = 0; i < 32; i++) {
    gprs.r_regs[i] = state.gp.regs[i];
  }
  gprs.r_mullo = state.special.lo;
  gprs.r_mulhi = state.special.hi;
  gprs.r_pc = state.special.pc;
  gprs.r_sr = state.cop0.status;
  gprs.r_badvaddr = state.cop0.badvaddr;
  gprs.r_cause = state.cop0.cause;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write FPU registers
  //
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;
  for (int i = 0; i < 32; i++) {
    fpregs.r_regs[i] = state.fpu.fpr[i];
  }
  fpregs.r_fcsr = state.fpu.fcsr;
  fpregs.r_fir = state.fpu.fir;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
