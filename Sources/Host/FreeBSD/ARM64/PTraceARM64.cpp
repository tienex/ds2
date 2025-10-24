//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::FreeBSD::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace FreeBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState &state) {
  pid_t pid = ptid.pid;
  struct reg gprs;

  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // FreeBSD ARM64 register layout
  for (int i = 0; i < 30; i++) {
    state.arm64.gp.regs[i] = gprs.x[i];
  }
  state.arm64.gp.fp = gprs.x[29];
  state.arm64.gp.lr = gprs.lr;
  state.arm64.gp.sp = gprs.sp;
  state.arm64.gp.pc = gprs.elr;
  state.arm64.gp.cpsr = gprs.spsr;

  // Floating-point state
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) >= 0) {
    for (int i = 0; i < 32; i++) {
      state.arm64.vfp.regs[i] = fprs.fp_q[i];
    }
    state.arm64.vfp.fpsr = fprs.fp_sr;
    state.arm64.vfp.fpcr = fprs.fp_cr;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;
  struct reg gprs;

  // Set general purpose registers
  for (int i = 0; i < 30; i++) {
    gprs.x[i] = state.arm64.gp.regs[i];
  }
  gprs.x[29] = state.arm64.gp.fp;
  gprs.lr = state.arm64.gp.lr;
  gprs.sp = state.arm64.gp.sp;
  gprs.elr = state.arm64.gp.pc;
  gprs.spsr = state.arm64.gp.cpsr;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // Set floating-point state
  struct fpreg fprs;
  for (int i = 0; i < 32; i++) {
    fprs.fp_q[i] = state.arm64.vfp.regs[i];
  }
  fprs.fp_sr = state.arm64.vfp.fpsr;
  fprs.fp_cr = state.arm64.vfp.fpcr;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
