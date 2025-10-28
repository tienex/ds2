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
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace OpenBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  // Read general-purpose and special registers
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // OpenBSD PowerPC64 register structure layout
  // Copy GPRs (r0-r31)
  for (size_t n = 0; n < 32; n++) {
    state.ppc64.gp.regs[n] = gprs.fixreg[n];
  }

  // Copy special registers
  state.ppc64.pc = gprs.pc;
  state.ppc64.lr = gprs.lr;
  state.ppc64.ctr = gprs.ctr;
  state.ppc64.cr = gprs.cr;
  state.ppc64.xer = gprs.xer;

  // Read floating-point registers
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.ppc64.fp.regs[n] = fprs.fpreg[n];
    }
    state.ppc64.fpscr = fprs.fpscr;
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
    gprs.fixreg[n] = state.ppc64.gp.regs[n];
  }

  gprs.pc = state.ppc64.pc;
  gprs.lr = state.ppc64.lr;
  gprs.ctr = state.ppc64.ctr;
  gprs.cr = state.ppc64.cr;
  gprs.xer = state.ppc64.xer;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (size_t n = 0; n < 32; n++) {
    fprs.fpreg[n] = state.ppc64.fp.regs[n];
  }
  fprs.fpscr = state.ppc64.fpscr;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    // Non-fatal if FPU not available
  }

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
