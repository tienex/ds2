//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Architecture/X86_64/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <elf.h>

using ds2::Host::Linux::PTrace;
using ds2::Host::Platform;

#define super ds2::Host::POSIX::PTrace

//
// x32 ABI: x86-64 with 32-bit pointers (ILP32 on x86-64)
// - Uses 64-bit registers and instructions
// - Pointers and longs are 32-bit
// - int, long, pointer = 32-bit
// - long long = 64-bit
// - Same register state as x86_64
//

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // x32 uses the same register layout as x86_64
  //
  struct user_regs_struct regs;
  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy general-purpose registers
  //
  state.gp.rax = regs.rax;
  state.gp.rbx = regs.rbx;
  state.gp.rcx = regs.rcx;
  state.gp.rdx = regs.rdx;
  state.gp.rsi = regs.rsi;
  state.gp.rdi = regs.rdi;
  state.gp.rbp = regs.rbp;
  state.gp.rsp = regs.rsp;
  state.gp.r8 = regs.r8;
  state.gp.r9 = regs.r9;
  state.gp.r10 = regs.r10;
  state.gp.r11 = regs.r11;
  state.gp.r12 = regs.r12;
  state.gp.r13 = regs.r13;
  state.gp.r14 = regs.r14;
  state.gp.r15 = regs.r15;
  state.gp.rip = regs.rip;
  state.gp.rflags = regs.eflags;

  //
  // Segment registers
  //
  state.segments.cs = regs.cs;
  state.segments.ss = regs.ss;
  state.segments.ds = regs.ds;
  state.segments.es = regs.es;
  state.segments.fs = regs.fs;
  state.segments.gs = regs.gs;
  state.segments.fs_base = regs.fs_base;
  state.segments.gs_base = regs.gs_base;

  //
  // Read FPU state
  //
  struct user_fpregs_struct fpregs;
  iov.iov_base = &fpregs;
  iov.iov_len = sizeof(fpregs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) >= 0) {
    // x87 FPU
    state.x87.fctrl = fpregs.cwd;
    state.x87.fstat = fpregs.swd;
    state.x87.ftag = fpregs.ftw;
    state.x87.fop = fpregs.fop;
    state.x87.fioff = fpregs.rip;
    state.x87.fooff = fpregs.rdp;

    for (int i = 0; i < 8; i++) {
      std::memcpy(&state.x87.fpregs[i], &fpregs.st_space[i * 4], 10);
    }

    // SSE registers
    for (int i = 0; i < 16; i++) {
      std::memcpy(&state.sse.regs[i], &fpregs.xmm_space[i * 4], 16);
    }
    state.sse.mxcsr = fpregs.mxcsr;
    state.sse.mxcsr_mask = fpregs.mxcsr_mask;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Write general-purpose registers
  //
  struct user_regs_struct regs;
  regs.rax = state.gp.rax;
  regs.rbx = state.gp.rbx;
  regs.rcx = state.gp.rcx;
  regs.rdx = state.gp.rdx;
  regs.rsi = state.gp.rsi;
  regs.rdi = state.gp.rdi;
  regs.rbp = state.gp.rbp;
  regs.rsp = state.gp.rsp;
  regs.r8 = state.gp.r8;
  regs.r9 = state.gp.r9;
  regs.r10 = state.gp.r10;
  regs.r11 = state.gp.r11;
  regs.r12 = state.gp.r12;
  regs.r13 = state.gp.r13;
  regs.r14 = state.gp.r14;
  regs.r15 = state.gp.r15;
  regs.rip = state.gp.rip;
  regs.eflags = state.gp.rflags;
  regs.cs = state.segments.cs;
  regs.ss = state.segments.ss;
  regs.ds = state.segments.ds;
  regs.es = state.segments.es;
  regs.fs = state.segments.fs;
  regs.gs = state.segments.gs;
  regs.fs_base = state.segments.fs_base;
  regs.gs_base = state.segments.gs_base;

  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write FPU state
  //
  struct user_fpregs_struct fpregs;
  std::memset(&fpregs, 0, sizeof(fpregs));

  fpregs.cwd = state.x87.fctrl;
  fpregs.swd = state.x87.fstat;
  fpregs.ftw = state.x87.ftag;
  fpregs.fop = state.x87.fop;
  fpregs.rip = state.x87.fioff;
  fpregs.rdp = state.x87.fooff;

  for (int i = 0; i < 8; i++) {
    std::memcpy(&fpregs.st_space[i * 4], &state.x87.fpregs[i], 10);
  }

  for (int i = 0; i < 16; i++) {
    std::memcpy(&fpregs.xmm_space[i * 4], &state.sse.regs[i], 16);
  }
  fpregs.mxcsr = state.sse.mxcsr;
  fpregs.mxcsr_mask = state.sse.mxcsr_mask;

  iov.iov_base = &fpregs;
  iov.iov_len = sizeof(fpregs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
