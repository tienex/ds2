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
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <asm/ptrace.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Read GPRs using PTRACE_GETREGS
  //
  struct pt_regs gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  //
  // Copy general purpose registers (r0-r31)
  // MIPS pt_regs layout: regs[32], lo, hi, cp0_epc, cp0_badvaddr, cp0_status, cp0_cause
  //
  std::memcpy(state.gp.regs, gprs.regs, sizeof(state.gp.regs));
  state.special.lo = gprs.lo;
  state.special.hi = gprs.hi;
  state.special.pc = gprs.cp0_epc;

  //
  // Copy CP0 registers
  //
  state.cop0.status = gprs.cp0_status;
  state.cop0.badvaddr = gprs.cp0_badvaddr;
  state.cop0.cause = gprs.cp0_cause;

#if defined(PTRACE_GETFPREGS)
  //
  // Read FPU registers if available
  //
  elf_fpregset_t fpregs;
  if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fpregs) >= 0) {
    std::memcpy(&state.fpu.sng, &fpregs.fpr, sizeof(state.fpu.sng));
    state.fpu.fcsr = fpregs.fcsr;
    state.fpu.fir = fpregs.fir;
  }
#endif

#if defined(PTRACE_GETDSPREGS)
  //
  // Read DSP registers if available
  //
  if (wrapPtrace(PTRACE_GETDSPREGS, pid, nullptr, &state.dsp) < 0) {
    // DSP not supported, clear registers
    std::memset(&state.dsp, 0, sizeof(state.dsp));
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
  struct pt_regs gprs;
  std::memcpy(gprs.regs, state.gp.regs, sizeof(state.gp.regs));
  gprs.lo = state.special.lo;
  gprs.hi = state.special.hi;
  gprs.cp0_epc = state.special.pc;
  gprs.cp0_status = state.cop0.status;
  gprs.cp0_badvaddr = state.cop0.badvaddr;
  gprs.cp0_cause = state.cop0.cause;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

#if defined(PTRACE_SETFPREGS)
  //
  // Write FPU registers
  //
  elf_fpregset_t fpregs;
  std::memcpy(&fpregs.fpr, &state.fpu.sng, sizeof(state.fpu.sng));
  fpregs.fcsr = state.fpu.fcsr;
  fpregs.fir = state.fpu.fir;

  if (wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fpregs) < 0)
    return Platform::TranslateError();
#endif

#if defined(PTRACE_SETDSPREGS)
  //
  // Write DSP registers if supported
  //
  wrapPtrace(PTRACE_SETDSPREGS, pid, nullptr, &state.dsp);
#endif

  return kSuccess;
}

//
// MIPS hardware breakpoint/watchpoint support
// Note: MIPS uses CP0 Debug registers for hardware breakpoints
//

int PTrace::getMaxHardwareBreakpoints(ProcessThreadId const &ptid) {
  // MIPS typically has 2-4 hardware breakpoints in debug mode
  // This needs to be queried from CP0_Debug register
  return 0; // Disabled by default, requires kernel support
}

int PTrace::getMaxHardwareWatchpoints(ProcessThreadId const &ptid) {
  // MIPS typically has 1-2 hardware watchpoints
  return 0; // Disabled by default, requires kernel support
}

int PTrace::getMaxWatchpointSize(ProcessThreadId const &ptid) {
  // MIPS watchpoints are typically word-aligned
  return 4;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
