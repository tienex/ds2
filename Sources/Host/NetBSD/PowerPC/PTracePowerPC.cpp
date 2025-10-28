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
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace NetBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  // Read general-purpose and special registers
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // NetBSD PowerPC register structure layout
  // Copy GPRs (r0-r31)
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = gprs.fixreg[n];
  }

  // Copy special registers
  state.ppc.pc = gprs.pc;
  state.ppc.lr = gprs.lr;
  state.ppc.ctr = gprs.ctr;
  state.ppc.cr = gprs.cr;
  state.ppc.xer = gprs.xer;

  // Read floating-point registers
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, &fprs, 0) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.ppc.fp.regs[n] = fprs.fpreg[n];
    }
    state.ppc.fpscr = fprs.fpscr;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;

  // Prepare general-purpose and special registers
  struct reg gprs;

  for (size_t n = 0; n < 32; n++) {
    gprs.fixreg[n] = state.ppc.gp.regs[n];
  }

  gprs.pc = state.ppc.pc;
  gprs.lr = state.ppc.lr;
  gprs.ctr = state.ppc.ctr;
  gprs.cr = state.ppc.cr;
  gprs.xer = state.ppc.xer;

  if (::ptrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (size_t n = 0; n < 32; n++) {
    fprs.fpreg[n] = state.ppc.fp.regs[n];
  }
  fprs.fpscr = state.ppc.fpscr;

  if (::ptrace(PT_SETFPREGS, pid, &fprs, 0) < 0) {
    // Non-fatal if FPU not available
  }

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
