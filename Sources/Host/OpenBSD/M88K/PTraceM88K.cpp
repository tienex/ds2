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
#include "DebugServer2/Architecture/M88K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

using ds2::Host::OpenBSD::PTrace;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace OpenBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  //
  // Read general-purpose registers
  // OpenBSD m88k struct reg layout
  //
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy m88k general-purpose registers (r0-r31)
  // OpenBSD struct reg has:
  // - r[32]: General registers
  // - epsr: Exception PSR
  // - fpsr: Floating-Point Status Register
  // - fpcr: Floating-Point Control Register
  // - sxip: Shadow Execute IP
  // - snip: Shadow Next IP
  // - sfip: Shadow Fetch IP
  // - ssbr: Shadow Scoreboard Register
  // - dmt0, dmd0, dma0: Data Memory Transaction 0
  // - dmt1, dmd1, dma1: Data Memory Transaction 1
  // - dmt2, dmd2, dma2: Data Memory Transaction 2
  // - fpecr: Floating-Point Exception Cause Register
  // - fphs1, fpls1: FP High/Low Significant part 1
  // - fphs2, fpls2: FP High/Low Significant part 2
  // - fppt: FP Precise Tag
  // - fprh, fprl: FP Result High/Low
  // - fpit: FP Imprecise Tag
  //
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gprs.r[i];
  }

  // Force r0 to zero (hardware constraint)
  state.gp.regs[0] = 0;

  //
  // Special registers
  //
  state.special.pc = gprs.sxip;     // Use SXIP as current PC
  state.special.npc = gprs.snip;    // Shadow Next IP
  state.special.fpecr = gprs.fpecr; // FP Exception Cause
  state.special.fpcr = gprs.fpcr;   // FP Control Register
  state.special.fpsr = gprs.fpsr;   // FP Status Register

  //
  // Processor Status Register
  // OpenBSD stores the EPSR (Exception PSR)
  //
  std::memcpy(&state.psr, &gprs.epsr, sizeof(state.psr));

  //
  // Control registers
  //
  state.cr.cr1 = gprs.epsr;   // PSR (stored as EPSR)
  state.cr.cr2 = gprs.epsr;   // EPSR
  state.cr.cr3 = gprs.ssbr;   // SSBR
  state.cr.cr4 = gprs.sxip;   // SXIP
  state.cr.cr5 = gprs.snip;   // SNIP
  state.cr.cr6 = gprs.sfip;   // SFIP

  // Data Memory Transaction registers
  state.cr.cr8 = gprs.dmt0;
  state.cr.cr9 = gprs.dmd0;
  state.cr.cr10 = gprs.dma0;
  state.cr.cr11 = gprs.dmt1;
  state.cr.cr12 = gprs.dmd1;
  state.cr.cr13 = gprs.dma1;
  state.cr.cr14 = gprs.dmt2;
  state.cr.cr15 = gprs.dmd2;
  state.cr.cr16 = gprs.dma2;

  //
  // Read floating-point registers
  //
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fpregs, 0) >= 0) {
    //
    // m88k FPU: 32 single-precision or 16 double-precision registers
    //
    for (int i = 0; i < 32; i++) {
      state.fpu.raw32[i] = fpregs.fp_regs[i];
    }
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

  for (int i = 0; i < 32; i++) {
    gprs.r[i] = state.gp.regs[i];
  }

  // Special registers
  gprs.sxip = state.special.pc;
  gprs.snip = state.special.npc;
  gprs.fpecr = state.special.fpecr;
  gprs.fpcr = state.special.fpcr;
  gprs.fpsr = state.special.fpsr;

  // PSR
  std::memcpy(&gprs.epsr, &state.psr, sizeof(gprs.epsr));

  // Control registers
  gprs.ssbr = state.cr.cr3;
  gprs.sfip = state.cr.cr6;

  // Data Memory Transaction registers
  gprs.dmt0 = state.cr.cr8;
  gprs.dmd0 = state.cr.cr9;
  gprs.dma0 = state.cr.cr10;
  gprs.dmt1 = state.cr.cr11;
  gprs.dmd1 = state.cr.cr12;
  gprs.dma1 = state.cr.cr13;
  gprs.dmt2 = state.cr.cr14;
  gprs.dmd2 = state.cr.cr15;
  gprs.dma2 = state.cr.cr16;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;

  for (int i = 0; i < 32; i++) {
    fpregs.fp_regs[i] = state.fpu.raw32[i];
  }

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
