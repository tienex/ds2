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
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Read GPRs using PT_GETREGS
  //
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  //
  // Copy MIPS general purpose registers
  // FreeBSD struct reg layout matches MIPS register layout
  //
  std::memcpy(state.gp.regs, gprs.r_regs, sizeof(state.gp.regs));
  state.special.lo = gprs.r_mullo;
  state.special.hi = gprs.r_mulhi;
  state.special.pc = gprs.r_pc;

  //
  // Copy status registers
  //
  state.cop0.status = gprs.r_sr;
  state.cop0.badvaddr = gprs.r_badvaddr;
  state.cop0.cause = gprs.r_cause;

#if defined(PT_GETFPREGS)
  //
  // Read FPU registers if available
  //
  struct fpreg fpregs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fpregs, 0) >= 0) {
    std::memcpy(&state.fpu.sng, &fpregs.r_regs, sizeof(state.fpu.sng));
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

  //
  // Write GPRs
  //
  struct reg gprs;
  std::memcpy(gprs.r_regs, state.gp.regs, sizeof(state.gp.regs));
  gprs.r_mullo = state.special.lo;
  gprs.r_mulhi = state.special.hi;
  gprs.r_pc = state.special.pc;
  gprs.r_sr = state.cop0.status;
  gprs.r_badvaddr = state.cop0.badvaddr;
  gprs.r_cause = state.cop0.cause;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

#if defined(PT_SETFPREGS)
  //
  // Write FPU registers
  //
  struct fpreg fpregs;
  std::memcpy(&fpregs.r_regs, &state.fpu.sng, sizeof(state.fpu.sng));
  fpregs.r_fcsr = state.fpu.fcsr;
  fpregs.r_fir = state.fpu.fir;

  if (wrapPtrace(PT_SETFPREGS, pid, &fpregs, 0) < 0)
    return Platform::TranslateError();
#endif

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
