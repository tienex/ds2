//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Linux Alpha PTrace CPU state
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Utils/Log.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

//
// Linux Alpha ptrace register structure
//
struct user_regs_struct {
  uint64_t r[32];       // Integer registers r0-r31
  uint64_t f[32];       // Floating-point registers f0-f31
  uint64_t pc;          // Program counter
  uint64_t fpcr;        // FP control register
  uint64_t unique;      // Thread pointer
};

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &info,
                                Architecture::CPUState &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (ptid.tid <= kAnyThreadId) {
    pid = ptid.pid;
  } else {
    pid = ptid.tid;
  }

  //
  // Read GPRs
  //
  struct user_regs_struct gprs;
  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  // Copy integer registers
  memcpy(&state.alpha.gp.regs, &gprs.r, sizeof(state.alpha.gp.regs));

  // Copy floating-point registers
  memcpy(&state.alpha.fp.raw, &gprs.f, sizeof(state.alpha.fp.raw));

  // Copy special registers
  state.alpha.pc = gprs.pc;
  state.alpha.fpcr = gprs.fpcr;
  state.alpha.unique = gprs.unique;

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &info,
                                 Architecture::CPUState const &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (ptid.tid <= kAnyThreadId) {
    pid = ptid.pid;
  } else {
    pid = ptid.tid;
  }

  //
  // Write GPRs
  //
  struct user_regs_struct gprs;

  // Copy integer registers
  memcpy(&gprs.r, &state.alpha.gp.regs, sizeof(gprs.r));

  // Copy floating-point registers
  memcpy(&gprs.f, &state.alpha.fp.raw, sizeof(gprs.f));

  // Copy special registers
  gprs.pc = state.alpha.pc;
  gprs.fpcr = state.alpha.fpcr;
  gprs.unique = state.alpha.unique;

  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
