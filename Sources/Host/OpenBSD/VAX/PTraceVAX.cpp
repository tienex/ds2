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
#include "DebugServer2/Architecture/VAX/CPUState.h"
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
  // OpenBSD VAX struct reg layout
  //
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy VAX general purpose registers (R0-R15)
  // OpenBSD struct reg has r[16], ap, fp, sp, pc, psl
  //
  for (int i = 0; i < 16; i++) {
    state.gp.regs[i] = gprs.r[i];
  }

  //
  // Copy Processor Status Longword
  //
  std::memcpy(&state.psl, &gprs.psl, sizeof(state.psl));

  //
  // Read floating-point registers if available
  // VAX supports multiple FP formats
  //
#if defined(PT_GETFPREGS)
  struct fpreg fpregs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fpregs, 0) >= 0) {
    //
    // OpenBSD VAX fpreg structure
    //

    // F-format (32-bit single precision)
    for (int i = 0; i < 16; i++) {
      state.f_float.f[i] = fpregs.f_regs[i];
    }

    // D-format (64-bit double precision)
    for (int i = 0; i < 16; i++) {
      state.d_float.d[i] = fpregs.d_regs[i];
    }

    // G-format (64-bit extended double precision)
    // Available on VAX-11/750 and later
    for (int i = 0; i < 16; i++) {
      state.g_float.g[i] = fpregs.g_regs[i];
    }

    // H-format (128-bit quad precision)
    // Available on VAX 6000/7000/8000/10000 series
    for (int i = 0; i < 16; i++) {
      state.h_float.h[i].low = fpregs.h_regs[i].low;
      state.h_float.h[i].high = fpregs.h_regs[i].high;
    }
  }
#endif

  //
  // Read system control registers if available
  // OpenBSD may provide these via extended register sets
  //
#if defined(PT_GETDBREGS)
  struct dbreg dbregs;
  if (::ptrace(PT_GETDBREGS, pid, (caddr_t)&dbregs, 0) >= 0) {
    state.system.ksp = dbregs.ksp;
    state.system.esp = dbregs.esp;
    state.system.ssp = dbregs.ssp;
    state.system.usp = dbregs.usp;
    state.system.isp = dbregs.isp;
    state.system.scbb = dbregs.scbb;
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
  for (int i = 0; i < 16; i++) {
    gprs.r[i] = state.gp.regs[i];
  }
  std::memcpy(&gprs.psl, &state.psl, sizeof(gprs.psl));

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
#if defined(PT_SETFPREGS)
  struct fpreg fpregs;

  // F-format
  for (int i = 0; i < 16; i++) {
    fpregs.f_regs[i] = state.f_float.f[i];
  }

  // D-format
  for (int i = 0; i < 16; i++) {
    fpregs.d_regs[i] = state.d_float.d[i];
  }

  // G-format
  for (int i = 0; i < 16; i++) {
    fpregs.g_regs[i] = state.g_float.g[i];
  }

  // H-format
  for (int i = 0; i < 16; i++) {
    fpregs.h_regs[i].low = state.h_float.h[i].low;
    fpregs.h_regs[i].high = state.h_float.h[i].high;
  }

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fpregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  //
  // Write system control registers if available
  //
#if defined(PT_SETDBREGS)
  struct dbreg dbregs;
  dbregs.ksp = state.system.ksp;
  dbregs.esp = state.system.esp;
  dbregs.ssp = state.system.ssp;
  dbregs.usp = state.system.usp;
  dbregs.isp = state.system.isp;
  dbregs.scbb = state.system.scbb;

  if (::ptrace(PT_SETDBREGS, pid, (caddr_t)&dbregs, 0) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
