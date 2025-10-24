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

  // NetBSD X86_64 register layout
  state.x86_64.gp.rax = gprs.regs[_REG_RAX];
  state.x86_64.gp.rbx = gprs.regs[_REG_RBX];
  state.x86_64.gp.rcx = gprs.regs[_REG_RCX];
  state.x86_64.gp.rdx = gprs.regs[_REG_RDX];
  state.x86_64.gp.rsi = gprs.regs[_REG_RSI];
  state.x86_64.gp.rdi = gprs.regs[_REG_RDI];
  state.x86_64.gp.rbp = gprs.regs[_REG_RBP];
  state.x86_64.gp.rsp = gprs.regs[_REG_RSP];
  state.x86_64.gp.r8 = gprs.regs[_REG_R8];
  state.x86_64.gp.r9 = gprs.regs[_REG_R9];
  state.x86_64.gp.r10 = gprs.regs[_REG_R10];
  state.x86_64.gp.r11 = gprs.regs[_REG_R11];
  state.x86_64.gp.r12 = gprs.regs[_REG_R12];
  state.x86_64.gp.r13 = gprs.regs[_REG_R13];
  state.x86_64.gp.r14 = gprs.regs[_REG_R14];
  state.x86_64.gp.r15 = gprs.regs[_REG_R15];
  state.x86_64.gp.rip = gprs.regs[_REG_RIP];
  state.x86_64.gp.rflags = gprs.regs[_REG_RFLAGS];
  state.x86_64.gp.cs = gprs.regs[_REG_CS];
  state.x86_64.gp.ss = gprs.regs[_REG_SS];
  state.x86_64.gp.ds = gprs.regs[_REG_DS];
  state.x86_64.gp.es = gprs.regs[_REG_ES];
  state.x86_64.gp.fs = gprs.regs[_REG_FS];
  state.x86_64.gp.gs = gprs.regs[_REG_GS];

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
  gprs.regs[_REG_RAX] = state.x86_64.gp.rax;
  gprs.regs[_REG_RBX] = state.x86_64.gp.rbx;
  gprs.regs[_REG_RCX] = state.x86_64.gp.rcx;
  gprs.regs[_REG_RDX] = state.x86_64.gp.rdx;
  gprs.regs[_REG_RSI] = state.x86_64.gp.rsi;
  gprs.regs[_REG_RDI] = state.x86_64.gp.rdi;
  gprs.regs[_REG_RBP] = state.x86_64.gp.rbp;
  gprs.regs[_REG_RSP] = state.x86_64.gp.rsp;
  gprs.regs[_REG_R8] = state.x86_64.gp.r8;
  gprs.regs[_REG_R9] = state.x86_64.gp.r9;
  gprs.regs[_REG_R10] = state.x86_64.gp.r10;
  gprs.regs[_REG_R11] = state.x86_64.gp.r11;
  gprs.regs[_REG_R12] = state.x86_64.gp.r12;
  gprs.regs[_REG_R13] = state.x86_64.gp.r13;
  gprs.regs[_REG_R14] = state.x86_64.gp.r14;
  gprs.regs[_REG_R15] = state.x86_64.gp.r15;
  gprs.regs[_REG_RIP] = state.x86_64.gp.rip;
  gprs.regs[_REG_RFLAGS] = state.x86_64.gp.rflags;
  gprs.regs[_REG_CS] = state.x86_64.gp.cs;
  gprs.regs[_REG_SS] = state.x86_64.gp.ss;
  gprs.regs[_REG_DS] = state.x86_64.gp.ds;
  gprs.regs[_REG_ES] = state.x86_64.gp.es;
  gprs.regs[_REG_FS] = state.x86_64.gp.fs;
  gprs.regs[_REG_GS] = state.x86_64.gp.gs;

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

} // namespace NetBSD
} // namespace Host
} // namespace ds2
