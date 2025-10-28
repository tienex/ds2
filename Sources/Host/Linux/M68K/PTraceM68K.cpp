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
#include "DebugServer2/Architecture/M68K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

using ds2::Host::Linux::PTrace;
using ds2::Host::Platform;

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Read general-purpose registers
  // Linux m68k uses struct pt_regs
  //
  struct pt_regs {
    uint32_t d[8];       // d0-d7
    uint32_t a[7];       // a0-a6
    uint32_t usp;        // user stack pointer
    uint32_t sr;         // status register
    uint32_t pc;         // program counter
    uint32_t orig_d0;    // original d0
    uint32_t stkadj;     // stack adjust
    uint16_t format;     // frame format
    uint16_t vector;     // exception vector
  } regs;

  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy data registers
  //
  for (int i = 0; i < 8; i++) {
    state.data.regs[i] = regs.d[i];
  }

  //
  // Copy address registers
  //
  for (int i = 0; i < 7; i++) {
    state.addr.regs[i] = regs.a[i];
  }

  //
  // Stack pointer and control registers
  //
  state.stack.usp = regs.usp;
  state.special.pc = regs.pc;
  state.special.sr = regs.sr;

  //
  // Copy SR bit fields
  //
  std::memcpy(&state.sr_flags, &regs.sr, sizeof(state.sr_flags));

  //
  // Read floating-point registers
  // Linux m68k FPU state
  //
  struct user_m68kfp_struct {
    uint32_t fpregs[24];     // 8 * 96-bit FP registers
    uint32_t fpcntl[3];      // fpcr, fpsr, fpiar
  } fpregs;

  iov.iov_base = &fpregs;
  iov.iov_len = sizeof(fpregs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) >= 0) {
    //
    // Copy FPU registers (80-bit extended precision)
    // Linux stores them as 96-bit (12 bytes) with padding
    //
    for (int i = 0; i < 8; i++) {
      uint32_t *fpreg = &fpregs.fpregs[i * 3];
      state.fpu.fpr[i].mantissa = ((uint64_t)fpreg[1] << 32) | fpreg[0];
      state.fpu.fpr[i].exponent = fpreg[2] & 0xFFFF;
    }

    // FPU control registers
    state.fpu.fpcr = fpregs.fpcntl[0];
    state.fpu.fpsr = fpregs.fpcntl[1];
    state.fpu.fpiar = fpregs.fpcntl[2];
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
  struct pt_regs {
    uint32_t d[8];
    uint32_t a[7];
    uint32_t usp;
    uint32_t sr;
    uint32_t pc;
    uint32_t orig_d0;
    uint32_t stkadj;
    uint16_t format;
    uint16_t vector;
  } regs;

  for (int i = 0; i < 8; i++) {
    regs.d[i] = state.data.regs[i];
  }

  for (int i = 0; i < 7; i++) {
    regs.a[i] = state.addr.regs[i];
  }

  regs.usp = state.stack.usp;
  regs.pc = state.special.pc;
  regs.sr = state.special.sr;
  regs.orig_d0 = 0;
  regs.stkadj = 0;
  regs.format = 0;
  regs.vector = 0;

  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
  struct user_m68kfp_struct {
    uint32_t fpregs[24];
    uint32_t fpcntl[3];
  } fpregs;

  for (int i = 0; i < 8; i++) {
    uint32_t *fpreg = &fpregs.fpregs[i * 3];
    fpreg[0] = state.fpu.fpr[i].mantissa & 0xFFFFFFFF;
    fpreg[1] = state.fpu.fpr[i].mantissa >> 32;
    fpreg[2] = state.fpu.fpr[i].exponent;
  }

  fpregs.fpcntl[0] = state.fpu.fpcr;
  fpregs.fpcntl[1] = state.fpu.fpsr;
  fpregs.fpcntl[2] = state.fpu.fpiar;

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
