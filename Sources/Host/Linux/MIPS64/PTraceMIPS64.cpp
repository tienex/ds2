//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Architecture/MIPS64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <asm/ptrace.h>
#include <elf.h>

#define super ds2::Host::POSIX::PTrace

using ds2::Host::Linux::PTrace;

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Read general-purpose registers
  struct pt_regs gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  // Copy GPRs (64-bit)
  std::memcpy(state.gp.regs, gprs.regs, sizeof(state.gp.regs));
  state.special.lo = gprs.lo;
  state.special.hi = gprs.hi;
  state.special.pc = gprs.cp0_epc;
  state.cop0.status = gprs.cp0_status;
  state.cop0.badvaddr = gprs.cp0_badvaddr;
  state.cop0.cause = gprs.cp0_cause;

  // Read FPU registers
#if defined(PTRACE_GETFPREGS)
  elf_fpregset_t fpregs;
  if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fpregs) >= 0) {
    std::memcpy(state.fpu.fpr, fpregs.fp_r.fp_dregs, sizeof(state.fpu.fpr));
    state.fpu.fcsr = fpregs.fp_csr;
    state.fpu.fir = fpregs.fp_fir;
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
  struct pt_regs gprs;
  std::memcpy(gprs.regs, state.gp.regs, sizeof(gprs.regs));
  gprs.lo = state.special.lo;
  gprs.hi = state.special.hi;
  gprs.cp0_epc = state.special.pc;
  gprs.cp0_status = state.cop0.status;
  gprs.cp0_badvaddr = state.cop0.badvaddr;
  gprs.cp0_cause = state.cop0.cause;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  // Write FPU registers
#if defined(PTRACE_SETFPREGS)
  elf_fpregset_t fpregs;
  std::memcpy(fpregs.fp_r.fp_dregs, state.fpu.fpr, sizeof(fpregs.fp_r.fp_dregs));
  fpregs.fp_csr = state.fpu.fcsr;
  fpregs.fp_fir = state.fpu.fir;

  if (wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fpregs) < 0)
    return Platform::TranslateError();
#endif

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
