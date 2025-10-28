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
#include "DebugServer2/Architecture/NS32K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::NetBSD::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace NetBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  //
  // Read general-purpose registers
  // NetBSD NS32K (pc532) struct reg layout
  //
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy NS32K general-purpose registers (r0-r7)
  // NetBSD struct reg for NS32K contains:
  // - r[8]: General registers r0-r7
  // - fp: Frame pointer
  // - sp: Stack pointer
  // - sb: Static base
  // - pc: Program counter
  // - psr: Processor status register
  // - mod: Module register
  //
  for (int i = 0; i < 8; i++) {
    state.gp.regs[i] = gprs.r_r[i];
  }

  //
  // Special/dedicated registers
  //
  state.special.fp = gprs.r_fp;
  state.special.sp = gprs.r_sp;
  state.special.sb = gprs.r_sb;
  state.special.pc = gprs.r_pc;
  state.special.psr = gprs.r_psr;
  state.special.mod = gprs.r_mod;

  //
  // Copy PSR bit fields
  //
  std::memcpy(&state.psr_flags, &gprs.r_psr, sizeof(state.psr_flags));

  //
  // Read floating-point registers
  // NetBSD NS32K FPU support (NS32081/NS32381)
  //
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fpregs, 0) >= 0) {
    //
    // NS32K FPU: 8 registers
    // Can be accessed as single or double precision
    //
    for (int i = 0; i < 8; i++) {
      state.fpu.raw32[i] = fpregs.r_freg[i];
    }

    // FPU Status Register
    state.fsr.rm = (fpregs.r_fsr & 0x3);
    state.fsr.uf = (fpregs.r_fsr >> 5) & 1;
    state.fsr.if_ = (fpregs.r_fsr >> 6) & 1;
    state.fsr.tt = (fpregs.r_fsr >> 7) & 1;
  }
#endif

  //
  // Read debug registers if available
  //
#if defined(PT_GETDBREGS)
  struct dbreg dbregs;
  if (wrapPtrace(PT_GETDBREGS, pid, &dbregs, 0) >= 0) {
    state.debug.bpc = dbregs.bpc;
    state.debug.dcr = dbregs.dcr;
    state.debug.dsr = dbregs.dsr;
    state.debug.car = dbregs.car;
  }
#endif

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;

  //
  // Write general-purpose registers
  //
  struct reg gprs;
  for (int i = 0; i < 8; i++) {
    gprs.r_r[i] = state.gp.regs[i];
  }

  gprs.r_fp = state.special.fp;
  gprs.r_sp = state.special.sp;
  gprs.r_sb = state.special.sb;
  gprs.r_pc = state.special.pc;
  gprs.r_psr = state.special.psr;
  gprs.r_mod = state.special.mod;

  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;

  for (int i = 0; i < 8; i++) {
    fpregs.r_freg[i] = state.fpu.raw32[i];
  }

  // Reconstruct FSR from bit fields
  fpregs.r_fsr = (state.fsr.rm & 0x3) |
                 ((state.fsr.uf & 1) << 5) |
                 ((state.fsr.if_ & 1) << 6) |
                 ((state.fsr.tt & 1) << 7);

  if (wrapPtrace(PT_SETFPREGS, pid, &fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  //
  // Write debug registers if available
  //
#if defined(PT_SETDBREGS)
  struct dbreg dbregs;
  dbregs.bpc = state.debug.bpc;
  dbregs.dcr = state.debug.dcr;
  dbregs.dsr = state.debug.dsr;
  dbregs.car = state.debug.car;

  if (wrapPtrace(PT_SETDBREGS, pid, &dbregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
