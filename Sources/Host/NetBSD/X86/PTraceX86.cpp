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
  struct reg gprs;

  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // NetBSD X86 register layout
  state.x86.gp.eax = gprs.r_eax;
  state.x86.gp.ebx = gprs.r_ebx;
  state.x86.gp.ecx = gprs.r_ecx;
  state.x86.gp.edx = gprs.r_edx;
  state.x86.gp.esi = gprs.r_esi;
  state.x86.gp.edi = gprs.r_edi;
  state.x86.gp.ebp = gprs.r_ebp;
  state.x86.gp.esp = gprs.r_esp;
  state.x86.gp.eip = gprs.r_eip;
  state.x86.gp.eflags = gprs.r_eflags;
  state.x86.gp.cs = gprs.r_cs;
  state.x86.gp.ss = gprs.r_ss;
  state.x86.gp.ds = gprs.r_ds;
  state.x86.gp.es = gprs.r_es;
  state.x86.gp.fs = gprs.r_fs;
  state.x86.gp.gs = gprs.r_gs;

  // Floating-point state
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) >= 0) {
    // Copy FPU state
    memcpy(&state.x86.fp, &fprs, sizeof(fprs));
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;
  struct reg gprs;

  // Set general purpose registers
  gprs.r_eax = state.x86.gp.eax;
  gprs.r_ebx = state.x86.gp.ebx;
  gprs.r_ecx = state.x86.gp.ecx;
  gprs.r_edx = state.x86.gp.edx;
  gprs.r_esi = state.x86.gp.esi;
  gprs.r_edi = state.x86.gp.edi;
  gprs.r_ebp = state.x86.gp.ebp;
  gprs.r_esp = state.x86.gp.esp;
  gprs.r_eip = state.x86.gp.eip;
  gprs.r_eflags = state.x86.gp.eflags;
  gprs.r_cs = state.x86.gp.cs;
  gprs.r_ss = state.x86.gp.ss;
  gprs.r_ds = state.x86.gp.ds;
  gprs.r_es = state.x86.gp.es;
  gprs.r_fs = state.x86.gp.fs;
  gprs.r_gs = state.x86.gp.gs;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // Set floating-point state
  struct fpreg fprs;
  memcpy(&fprs, &state.x86.fp, sizeof(fprs));
  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
