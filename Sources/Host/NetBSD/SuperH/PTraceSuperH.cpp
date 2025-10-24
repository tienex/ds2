//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/SuperH/CPUState.h"
#include "DebugServer2/Host/NetBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace NetBSD {

using namespace Architecture::SuperH;

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
  for (int i = 0; i < 16; i++) {
    state.gp.regs[i] = gprs.r_regs[i];
  }

  state.special.pc = gprs.r_pc;
  state.special.pr = gprs.r_pr;
  state.special.sr = gprs.r_sr;
  state.special.gbr = gprs.r_gbr;
  state.special.mach = gprs.r_mach;
  state.special.macl = gprs.r_macl;
  state.special.vbr = gprs.r_vbr;

  // Read floating-point registers
  struct fpreg fprs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fprs, 0) < 0) {
    // FPU might not be present or enabled, this is not fatal
    memset(&state.fpu, 0, sizeof(state.fpu));
    memset(&state.fpu_ctrl, 0, sizeof(state.fpu_ctrl));
    return kSuccess;
  }

  // FR0-FR15 (bank 0)
  for (int i = 0; i < 16; i++) {
    state.fpu.w[i] = fprs.fpscr_fr.fr_regs[i];
  }

  // XF0-XF15 (bank 1)
  for (int i = 0; i < 16; i++) {
    state.fpu.w[i + 16] = fprs.fpscr_fr.xf_regs[i];
  }

  state.fpu_ctrl.fpscr = fprs.fpscr_fr.fpscr;
  state.fpu_ctrl.fpul = fprs.fpul;

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

  for (int i = 0; i < 16; i++) {
    gprs.r_regs[i] = state.gp.regs[i];
  }

  gprs.r_pc = state.special.pc;
  gprs.r_pr = state.special.pr;
  gprs.r_sr = state.special.sr;
  gprs.r_gbr = state.special.gbr;
  gprs.r_mach = state.special.mach;
  gprs.r_macl = state.special.macl;
  gprs.r_vbr = state.special.vbr;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  // FR0-FR15 (bank 0)
  for (int i = 0; i < 16; i++) {
    fprs.fpscr_fr.fr_regs[i] = state.fpu.w[i];
  }

  // XF0-XF15 (bank 1)
  for (int i = 0; i < 16; i++) {
    fprs.fpscr_fr.xf_regs[i] = state.fpu.w[i + 16];
  }

  fprs.fpscr_fr.fpscr = state.fpu_ctrl.fpscr;
  fprs.fpul = state.fpu_ctrl.fpul;

  if (wrapPtrace(PT_SETFPREGS, pid, &fprs, 0) < 0) {
    // FPU might not be present or enabled, this is not fatal
    return kSuccess;
  }

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
