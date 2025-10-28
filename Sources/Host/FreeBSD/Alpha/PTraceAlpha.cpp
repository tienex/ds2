//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// FreeBSD Alpha PTrace CPU state
//

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &info,
                                Architecture::CPUState &state) {
  pid_t pid = ptid.tid <= kAnyThreadId ? ptid.pid : ptid.tid;

  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // Copy integer registers
  memcpy(&state.alpha.gp.regs, &gprs.r_regs, sizeof(state.alpha.gp.regs));
  state.alpha.pc = gprs.r_pc;

  // FreeBSD might have FP state in a separate structure
  struct fpreg fprs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fprs, 0) == 0) {
    memcpy(&state.alpha.fp.raw, &fprs.fpr_regs, sizeof(state.alpha.fp.raw));
    state.alpha.fpcr = fprs.fpr_cr;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &info,
                                 Architecture::CPUState const &state) {
  pid_t pid = ptid.tid <= kAnyThreadId ? ptid.pid : ptid.tid;

  struct reg gprs;
  memcpy(&gprs.r_regs, &state.alpha.gp.regs, sizeof(gprs.r_regs));
  gprs.r_pc = state.alpha.pc;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  struct fpreg fprs;
  memcpy(&fprs.fpr_regs, &state.alpha.fp.raw, sizeof(fprs.fpr_regs));
  fprs.fpr_cr = state.alpha.fpcr;

  if (wrapPtrace(PT_SETFPREGS, pid, &fprs, 0) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
