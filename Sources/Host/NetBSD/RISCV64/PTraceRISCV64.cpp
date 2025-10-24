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
#include "DebugServer2/Host/NetBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace NetBSD {

using namespace Architecture::RISCV64;

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                                Architecture::CPUState &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Read general purpose registers
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Map NetBSD's reg structure to our CPUState
  // x0 is always zero
  state.gp.regs[0] = 0;

  // x1-x31
  state.gp.regs[1] = gprs.r_ra;
  state.gp.regs[2] = gprs.r_sp;
  state.gp.regs[3] = gprs.r_gp;
  state.gp.regs[4] = gprs.r_tp;
  state.gp.regs[5] = gprs.r_t[0];
  state.gp.regs[6] = gprs.r_t[1];
  state.gp.regs[7] = gprs.r_t[2];
  state.gp.regs[8] = gprs.r_s[0];
  state.gp.regs[9] = gprs.r_s[1];
  state.gp.regs[10] = gprs.r_a[0];
  state.gp.regs[11] = gprs.r_a[1];
  state.gp.regs[12] = gprs.r_a[2];
  state.gp.regs[13] = gprs.r_a[3];
  state.gp.regs[14] = gprs.r_a[4];
  state.gp.regs[15] = gprs.r_a[5];
  state.gp.regs[16] = gprs.r_a[6];
  state.gp.regs[17] = gprs.r_a[7];
  state.gp.regs[18] = gprs.r_s[2];
  state.gp.regs[19] = gprs.r_s[3];
  state.gp.regs[20] = gprs.r_s[4];
  state.gp.regs[21] = gprs.r_s[5];
  state.gp.regs[22] = gprs.r_s[6];
  state.gp.regs[23] = gprs.r_s[7];
  state.gp.regs[24] = gprs.r_s[8];
  state.gp.regs[25] = gprs.r_s[9];
  state.gp.regs[26] = gprs.r_s[10];
  state.gp.regs[27] = gprs.r_s[11];
  state.gp.regs[28] = gprs.r_t[3];
  state.gp.regs[29] = gprs.r_t[4];
  state.gp.regs[30] = gprs.r_t[5];
  state.gp.regs[31] = gprs.r_t[6];

  state.special.pc = gprs.r_pc;

  // Read floating-point registers
  struct fpreg fprs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fprs, 0) < 0)
    return Platform::TranslateError();

  for (int i = 0; i < 32; i++) {
    state.fpu.l[i] = fprs.f_fpr[i];
  }
  state.fpu_ctrl.fcsr = fprs.f_fcsr;

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

  // Map our CPUState to NetBSD's reg structure
  // x0 is hardwired to zero, don't write it
  gprs.r_ra = state.gp.regs[1];
  gprs.r_sp = state.gp.regs[2];
  gprs.r_gp = state.gp.regs[3];
  gprs.r_tp = state.gp.regs[4];
  gprs.r_t[0] = state.gp.regs[5];
  gprs.r_t[1] = state.gp.regs[6];
  gprs.r_t[2] = state.gp.regs[7];
  gprs.r_s[0] = state.gp.regs[8];
  gprs.r_s[1] = state.gp.regs[9];
  gprs.r_a[0] = state.gp.regs[10];
  gprs.r_a[1] = state.gp.regs[11];
  gprs.r_a[2] = state.gp.regs[12];
  gprs.r_a[3] = state.gp.regs[13];
  gprs.r_a[4] = state.gp.regs[14];
  gprs.r_a[5] = state.gp.regs[15];
  gprs.r_a[6] = state.gp.regs[16];
  gprs.r_a[7] = state.gp.regs[17];
  gprs.r_s[2] = state.gp.regs[18];
  gprs.r_s[3] = state.gp.regs[19];
  gprs.r_s[4] = state.gp.regs[20];
  gprs.r_s[5] = state.gp.regs[21];
  gprs.r_s[6] = state.gp.regs[22];
  gprs.r_s[7] = state.gp.regs[23];
  gprs.r_s[8] = state.gp.regs[24];
  gprs.r_s[9] = state.gp.regs[25];
  gprs.r_s[10] = state.gp.regs[26];
  gprs.r_s[11] = state.gp.regs[27];
  gprs.r_t[3] = state.gp.regs[28];
  gprs.r_t[4] = state.gp.regs[29];
  gprs.r_t[5] = state.gp.regs[30];
  gprs.r_t[6] = state.gp.regs[31];

  gprs.r_pc = state.special.pc;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (int i = 0; i < 32; i++) {
    fprs.f_fpr[i] = state.fpu.l[i];
  }
  fprs.f_fcsr = state.fpu_ctrl.fcsr;

  if (wrapPtrace(PT_SETFPREGS, pid, &fprs, 0) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
