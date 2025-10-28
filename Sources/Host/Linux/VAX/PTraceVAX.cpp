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
#include "DebugServer2/Architecture/VAX/CPUState.h"
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
  // VAX has 16 32-bit GPRs (R0-R15)
  //
  struct user_regs_struct {
    uint32_t r[16];     // R0-R15 (includes AP, FP, SP, PC)
    uint32_t psl;       // Processor Status Longword
  } gprs;

  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Copy VAX general purpose registers
  //
  for (int i = 0; i < 16; i++) {
    state.gp.regs[i] = gprs.r[i];
  }

  //
  // Copy Processor Status Longword
  //
  std::memcpy(&state.psl, &gprs.psl, sizeof(state.psl));

  //
  // Read floating-point registers
  // VAX supports multiple FP formats
  //
  struct user_fpregs_struct {
    uint32_t f_regs[16];   // F-format (32-bit)
    uint64_t d_regs[16];   // D-format (64-bit)
    uint64_t g_regs[16];   // G-format (64-bit extended)
    struct {
      uint64_t low;
      uint64_t high;
    } h_regs[16];          // H-format (128-bit quad)
  } fpregs;

  iov.iov_base = &fpregs;
  iov.iov_len = sizeof(fpregs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) >= 0) {
    //
    // Copy F-format registers (32-bit single precision)
    //
    for (int i = 0; i < 16; i++) {
      state.f_float.f[i] = fpregs.f_regs[i];
    }

    //
    // Copy D-format registers (64-bit double precision)
    //
    for (int i = 0; i < 16; i++) {
      state.d_float.d[i] = fpregs.d_regs[i];
    }

    //
    // Copy G-format registers (64-bit extended double precision)
    //
    for (int i = 0; i < 16; i++) {
      state.g_float.g[i] = fpregs.g_regs[i];
    }

    //
    // Copy H-format registers (128-bit quad precision)
    //
    for (int i = 0; i < 16; i++) {
      state.h_float.h[i].low = fpregs.h_regs[i].low;
      state.h_float.h[i].high = fpregs.h_regs[i].high;
    }
  }

  //
  // Read system control registers if available
  //
#if defined(NT_VAX_SYSCTRL)
  struct user_sysctrl_struct {
    uint32_t ksp;     // Kernel Stack Pointer
    uint32_t esp;     // Executive Stack Pointer
    uint32_t ssp;     // Supervisor Stack Pointer
    uint32_t usp;     // User Stack Pointer
    uint32_t isp;     // Interrupt Stack Pointer
    uint32_t scbb;    // System Control Block Base
  } sysctrl;

  iov.iov_base = &sysctrl;
  iov.iov_len = sizeof(sysctrl);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_VAX_SYSCTRL, &iov) >= 0) {
    state.system.ksp = sysctrl.ksp;
    state.system.esp = sysctrl.esp;
    state.system.ssp = sysctrl.ssp;
    state.system.usp = sysctrl.usp;
    state.system.isp = sysctrl.isp;
    state.system.scbb = sysctrl.scbb;
  }
#endif

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
  struct user_regs_struct {
    uint32_t r[16];
    uint32_t psl;
  } gprs;

  for (int i = 0; i < 16; i++) {
    gprs.r[i] = state.gp.regs[i];
  }
  std::memcpy(&gprs.psl, &state.psl, sizeof(gprs.psl));

  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
  struct user_fpregs_struct {
    uint32_t f_regs[16];
    uint64_t d_regs[16];
    uint64_t g_regs[16];
    struct {
      uint64_t low;
      uint64_t high;
    } h_regs[16];
  } fpregs;

  for (int i = 0; i < 16; i++) {
    fpregs.f_regs[i] = state.f_float.f[i];
    fpregs.d_regs[i] = state.d_float.d[i];
    fpregs.g_regs[i] = state.g_float.g[i];
    fpregs.h_regs[i].low = state.h_float.h[i].low;
    fpregs.h_regs[i].high = state.h_float.h[i].high;
  }

  iov.iov_base = &fpregs;
  iov.iov_len = sizeof(fpregs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0) {
    return Platform::TranslateError();
  }

  //
  // Write system control registers if available
  //
#if defined(NT_VAX_SYSCTRL)
  struct user_sysctrl_struct {
    uint32_t ksp, esp, ssp, usp, isp, scbb;
  } sysctrl;

  sysctrl.ksp = state.system.ksp;
  sysctrl.esp = state.system.esp;
  sysctrl.ssp = state.system.ssp;
  sysctrl.usp = state.system.usp;
  sysctrl.isp = state.system.isp;
  sysctrl.scbb = state.system.scbb;

  iov.iov_base = &sysctrl;
  iov.iov_len = sizeof(sysctrl);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_VAX_SYSCTRL, &iov) < 0) {
    return Platform::TranslateError();
  }
#endif

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
