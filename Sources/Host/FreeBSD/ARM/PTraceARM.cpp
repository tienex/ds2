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

  // FreeBSD ARM register layout
  for (int i = 0; i < 13; i++) {
    state.arm.gp.regs[i] = gprs.r[i];
  }
  state.arm.gp.sp = gprs.r_sp;
  state.arm.gp.lr = gprs.r_lr;
  state.arm.gp.pc = gprs.r_pc;
  state.arm.gp.cpsr = gprs.r_cpsr;

  // Floating-point state
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) >= 0) {
    // VFP registers
    for (int i = 0; i < 32; i++) {
      state.arm.vfp.sd[i] = fprs.fpr_vfp.vfp_reg[i];
    }
    state.arm.vfp.fpscr = fprs.fpr_vfp.vfp_scr;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;
  struct reg gprs;

  // Set general purpose registers
  for (int i = 0; i < 13; i++) {
    gprs.r[i] = state.arm.gp.regs[i];
  }
  gprs.r_sp = state.arm.gp.sp;
  gprs.r_lr = state.arm.gp.lr;
  gprs.r_pc = state.arm.gp.pc;
  gprs.r_cpsr = state.arm.gp.cpsr;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // Set floating-point state
  struct fpreg fprs;
  for (int i = 0; i < 32; i++) {
    fprs.fpr_vfp.vfp_reg[i] = state.arm.vfp.sd[i];
  }
  fprs.fpr_vfp.vfp_scr = state.arm.vfp.fpscr;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
