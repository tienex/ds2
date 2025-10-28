//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PARISC/CPUState.h"
#include "DebugServer2/Host/OpenBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace OpenBSD {

using namespace Architecture::PARISC;

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Read general purpose registers
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // Map OpenBSD's reg structure to our CPUState
  // OpenBSD hppa reg structure contains:
  // r0-r31, sar, pcoqh, pcoqt

  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gprs.r_regs[i];
  }

  state.special.sar = gprs.r_sar;
  state.special.pcoq_head = gprs.r_pcoqh;
  state.special.pcoq_tail = gprs.r_pcoqt;

  // IAOQ is same as PCOQ on PA-RISC
  state.special.iaoq_head = gprs.r_pcoqh;
  state.special.iaoq_tail = gprs.r_pcoqt;

  // Read floating-point registers
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    // FPU might not be present, this is not fatal
    memset(&state.fpu, 0, sizeof(state.fpu));
    memset(&state.fpu_status, 0, sizeof(state.fpu_status));
    return kSuccess;
  }

  for (int i = 0; i < 32; i++) {
    state.fpu.l[i] = fprs.fpr_regs[i];
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &pinfo,
                                 Architecture::CPUState const &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Write general purpose registers
  struct reg gprs;

  for (int i = 0; i < 32; i++) {
    gprs.r_regs[i] = state.gp.regs[i];
  }

  gprs.r_sar = state.special.sar;
  gprs.r_pcoqh = state.special.pcoq_head;
  gprs.r_pcoqt = state.special.pcoq_tail;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (int i = 0; i < 32; i++) {
    fprs.fpr_regs[i] = state.fpu.l[i];
  }

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    // FPU might not be present, this is not fatal
    return kSuccess;
  }

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
