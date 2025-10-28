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
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

// Linux PowerPC ptrace register layout
struct ppc_pt_regs {
  uint32_t gpr[32];
  uint32_t nip;
  uint32_t msr;
  uint32_t orig_gpr3;
  uint32_t ctr;
  uint32_t link;
  uint32_t xer;
  uint32_t ccr;
  uint32_t mq;
  uint32_t trap;
  uint32_t dar;
  uint32_t dsisr;
  uint32_t result;
};

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Read GPRs and special registers
  struct ppc_pt_regs regs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &regs) < 0)
    return Platform::TranslateError();

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = regs.gpr[n];
  }

  // Copy special registers
  state.ppc.pc = regs.nip;
  state.ppc.msr = regs.msr;
  state.ppc.ctr = regs.ctr;
  state.ppc.lr = regs.link;
  state.ppc.xer = regs.xer;
  state.ppc.cr = regs.ccr;
  state.ppc.dar = regs.dar;
  state.ppc.dsisr = regs.dsisr;

  // Read FPRs
  struct iovec iov;
  double fprs[32];
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRFPREG, &iov) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.ppc.fp.regs[n] = fprs[n];
    }
  }

  // Read AltiVec/VMX registers if available
  struct {
    uint32_t vr[32][4];  // 32 x 128-bit vector registers
    uint32_t vscr;
    uint32_t vrsave;
  } vrregs;

  iov.iov_base = &vrregs;
  iov.iov_len = sizeof(vrregs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PPC_VMX, &iov) == 0) {
    for (size_t n = 0; n < 32; n++) {
      for (size_t i = 0; i < 4; i++) {
        state.ppc.vr[n].v[i] = vrregs.vr[n][i];
      }
    }
    state.ppc.vscr = vrregs.vscr;
    state.ppc.vrsave = vrregs.vrsave;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Prepare GPRs and special registers
  struct ppc_pt_regs regs;

  for (size_t n = 0; n < 32; n++) {
    regs.gpr[n] = state.ppc.gp.regs[n];
  }

  regs.nip = state.ppc.pc;
  regs.msr = state.ppc.msr;
  regs.ctr = state.ppc.ctr;
  regs.link = state.ppc.lr;
  regs.xer = state.ppc.xer;
  regs.ccr = state.ppc.cr;
  regs.dar = state.ppc.dar;
  regs.dsisr = state.ppc.dsisr;
  regs.orig_gpr3 = 0;
  regs.trap = 0;
  regs.result = 0;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &regs) < 0)
    return Platform::TranslateError();

  // Write FPRs
  struct iovec iov;
  double fprs[32];

  for (size_t n = 0; n < 32; n++) {
    fprs[n] = state.ppc.fp.regs[n];
  }

  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRFPREG, &iov) < 0) {
    // Non-fatal if FPU not available
  }

  // Write AltiVec/VMX registers if available
  struct {
    uint32_t vr[32][4];
    uint32_t vscr;
    uint32_t vrsave;
  } vrregs;

  for (size_t n = 0; n < 32; n++) {
    for (size_t i = 0; i < 4; i++) {
      vrregs.vr[n][i] = state.ppc.vr[n].v[i];
    }
  }
  vrregs.vscr = state.ppc.vscr;
  vrregs.vrsave = state.ppc.vrsave;

  iov.iov_base = &vrregs;
  iov.iov_len = sizeof(vrregs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PPC_VMX, &iov) < 0) {
    // Non-fatal if AltiVec not available
  }

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
