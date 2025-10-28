//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/RISCV64/CPUState.h"
#include "DebugServer2/Host/OpenBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace OpenBSD {

using namespace Architecture::RISCV64;

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
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
  // x0 is always zero
  state.gp.regs[0] = 0;

  // x1-x31
  state.gp.regs[1] = gprs.ra;
  state.gp.regs[2] = gprs.sp;
  state.gp.regs[3] = gprs.gp;
  state.gp.regs[4] = gprs.tp;
  state.gp.regs[5] = gprs.t[0];
  state.gp.regs[6] = gprs.t[1];
  state.gp.regs[7] = gprs.t[2];
  state.gp.regs[8] = gprs.s[0];
  state.gp.regs[9] = gprs.s[1];
  state.gp.regs[10] = gprs.a[0];
  state.gp.regs[11] = gprs.a[1];
  state.gp.regs[12] = gprs.a[2];
  state.gp.regs[13] = gprs.a[3];
  state.gp.regs[14] = gprs.a[4];
  state.gp.regs[15] = gprs.a[5];
  state.gp.regs[16] = gprs.a[6];
  state.gp.regs[17] = gprs.a[7];
  state.gp.regs[18] = gprs.s[2];
  state.gp.regs[19] = gprs.s[3];
  state.gp.regs[20] = gprs.s[4];
  state.gp.regs[21] = gprs.s[5];
  state.gp.regs[22] = gprs.s[6];
  state.gp.regs[23] = gprs.s[7];
  state.gp.regs[24] = gprs.s[8];
  state.gp.regs[25] = gprs.s[9];
  state.gp.regs[26] = gprs.s[10];
  state.gp.regs[27] = gprs.s[11];
  state.gp.regs[28] = gprs.t[3];
  state.gp.regs[29] = gprs.t[4];
  state.gp.regs[30] = gprs.t[5];
  state.gp.regs[31] = gprs.t[6];

  state.special.pc = gprs.sepc;

  // Read floating-point registers
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) < 0)
    return Platform::TranslateError();

  for (int i = 0; i < 32; i++) {
    state.fpu.l[i] = fprs.fp_x[i];
  }
  state.fpu_ctrl.fcsr = fprs.fp_fcsr;

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

  // Map our CPUState to OpenBSD's reg structure
  // x0 is hardwired to zero, don't write it
  gprs.ra = state.gp.regs[1];
  gprs.sp = state.gp.regs[2];
  gprs.gp = state.gp.regs[3];
  gprs.tp = state.gp.regs[4];
  gprs.t[0] = state.gp.regs[5];
  gprs.t[1] = state.gp.regs[6];
  gprs.t[2] = state.gp.regs[7];
  gprs.s[0] = state.gp.regs[8];
  gprs.s[1] = state.gp.regs[9];
  gprs.a[0] = state.gp.regs[10];
  gprs.a[1] = state.gp.regs[11];
  gprs.a[2] = state.gp.regs[12];
  gprs.a[3] = state.gp.regs[13];
  gprs.a[4] = state.gp.regs[14];
  gprs.a[5] = state.gp.regs[15];
  gprs.a[6] = state.gp.regs[16];
  gprs.a[7] = state.gp.regs[17];
  gprs.s[2] = state.gp.regs[18];
  gprs.s[3] = state.gp.regs[19];
  gprs.s[4] = state.gp.regs[20];
  gprs.s[5] = state.gp.regs[21];
  gprs.s[6] = state.gp.regs[22];
  gprs.s[7] = state.gp.regs[23];
  gprs.s[8] = state.gp.regs[24];
  gprs.s[9] = state.gp.regs[25];
  gprs.s[10] = state.gp.regs[26];
  gprs.s[11] = state.gp.regs[27];
  gprs.t[3] = state.gp.regs[28];
  gprs.t[4] = state.gp.regs[29];
  gprs.t[5] = state.gp.regs[30];
  gprs.t[6] = state.gp.regs[31];

  gprs.sepc = state.special.pc;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (int i = 0; i < 32; i++) {
    fprs.fp_x[i] = state.fpu.l[i];
  }
  fprs.fp_fcsr = state.fpu_ctrl.fcsr;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
