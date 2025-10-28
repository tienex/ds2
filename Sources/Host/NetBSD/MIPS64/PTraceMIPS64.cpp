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
#include "DebugServer2/Architecture/MIPS64/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace NetBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Read general-purpose registers
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Copy GPRs (64-bit)
  std::memcpy(state.gp.regs, gprs.r_regs, sizeof(state.gp.regs));
  state.special.lo = gprs.r_mullo;
  state.special.hi = gprs.r_mulhi;
  state.special.pc = gprs.r_pc;
  state.cop0.status = gprs.r_sr;
  state.cop0.badvaddr = gprs.r_badvaddr;
  state.cop0.cause = gprs.r_cause;

  // Read FPU registers
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fpregs, 0) >= 0) {
    std::memcpy(state.fpu.fpr, fpregs.r_regs, sizeof(state.fpu.fpr));
    state.fpu.fcsr = fpregs.r_fcsr;
    state.fpu.fir = fpregs.r_fir;
  }
#endif

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Write general-purpose registers
  struct reg gprs;
  std::memcpy(gprs.r_regs, state.gp.regs, sizeof(gprs.r_regs));
  gprs.r_mullo = state.special.lo;
  gprs.r_mulhi = state.special.hi;
  gprs.r_pc = state.special.pc;
  gprs.r_sr = state.cop0.status;
  gprs.r_badvaddr = state.cop0.badvaddr;
  gprs.r_cause = state.cop0.cause;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Write FPU registers
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;
  std::memcpy(fpregs.r_regs, state.fpu.fpr, sizeof(fpregs.r_regs));
  fpregs.r_fcsr = state.fpu.fcsr;
  fpregs.r_fir = state.fpu.fir;

  if (wrapPtrace(PT_SETFPREGS, pid, &fpregs, 0) < 0)
    return Platform::TranslateError();
#endif

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
