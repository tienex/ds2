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
  struct reg gprs;

  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // OpenBSD X86_64 register layout
  state.x86_64.gp.rax = gprs.r_rax;
  state.x86_64.gp.rbx = gprs.r_rbx;
  state.x86_64.gp.rcx = gprs.r_rcx;
  state.x86_64.gp.rdx = gprs.r_rdx;
  state.x86_64.gp.rsi = gprs.r_rsi;
  state.x86_64.gp.rdi = gprs.r_rdi;
  state.x86_64.gp.rbp = gprs.r_rbp;
  state.x86_64.gp.rsp = gprs.r_rsp;
  state.x86_64.gp.r8 = gprs.r_r8;
  state.x86_64.gp.r9 = gprs.r_r9;
  state.x86_64.gp.r10 = gprs.r_r10;
  state.x86_64.gp.r11 = gprs.r_r11;
  state.x86_64.gp.r12 = gprs.r_r12;
  state.x86_64.gp.r13 = gprs.r_r13;
  state.x86_64.gp.r14 = gprs.r_r14;
  state.x86_64.gp.r15 = gprs.r_r15;
  state.x86_64.gp.rip = gprs.r_rip;
  state.x86_64.gp.rflags = gprs.r_rflags;
  state.x86_64.gp.cs = gprs.r_cs;
  state.x86_64.gp.ss = gprs.r_ss;
  state.x86_64.gp.ds = gprs.r_ds;
  state.x86_64.gp.es = gprs.r_es;
  state.x86_64.gp.fs = gprs.r_fs;
  state.x86_64.gp.gs = gprs.r_gs;

  // Floating-point state
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) >= 0) {
    memcpy(&state.x86_64.fp, &fprs, sizeof(fprs));
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;
  struct reg gprs;

  // Set general purpose registers
  gprs.r_rax = state.x86_64.gp.rax;
  gprs.r_rbx = state.x86_64.gp.rbx;
  gprs.r_rcx = state.x86_64.gp.rcx;
  gprs.r_rdx = state.x86_64.gp.rdx;
  gprs.r_rsi = state.x86_64.gp.rsi;
  gprs.r_rdi = state.x86_64.gp.rdi;
  gprs.r_rbp = state.x86_64.gp.rbp;
  gprs.r_rsp = state.x86_64.gp.rsp;
  gprs.r_r8 = state.x86_64.gp.r8;
  gprs.r_r9 = state.x86_64.gp.r9;
  gprs.r_r10 = state.x86_64.gp.r10;
  gprs.r_r11 = state.x86_64.gp.r11;
  gprs.r_r12 = state.x86_64.gp.r12;
  gprs.r_r13 = state.x86_64.gp.r13;
  gprs.r_r14 = state.x86_64.gp.r14;
  gprs.r_r15 = state.x86_64.gp.r15;
  gprs.r_rip = state.x86_64.gp.rip;
  gprs.r_rflags = state.x86_64.gp.rflags;
  gprs.r_cs = state.x86_64.gp.cs;
  gprs.r_ss = state.x86_64.gp.ss;
  gprs.r_ds = state.x86_64.gp.ds;
  gprs.r_es = state.x86_64.gp.es;
  gprs.r_fs = state.x86_64.gp.fs;
  gprs.r_gs = state.x86_64.gp.gs;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0) {
    return Platform::TranslateError();
  }

  // Set floating-point state
  struct fpreg fprs;
  memcpy(&fprs, &state.x86_64.fp, sizeof(fprs));
  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace OpenBSD
} // namespace Host
} // namespace ds2
