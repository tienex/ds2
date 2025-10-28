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

// Linux PowerPC64 ptrace register layout
struct ppc64_pt_regs {
  uint64_t gpr[32];
  uint64_t nip;
  uint64_t msr;
  uint64_t orig_gpr3;
  uint64_t ctr;
  uint64_t link;
  uint64_t xer;
  uint64_t ccr;
  uint64_t softe;
  uint64_t trap;
  uint64_t dar;
  uint64_t dsisr;
  uint64_t result;
};

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Read GPRs and special registers
  struct ppc64_pt_regs regs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &regs) < 0)
    return Platform::TranslateError();

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    state.ppc64.gp.regs[n] = regs.gpr[n];
  }

  // Copy special registers
  state.ppc64.pc = regs.nip;
  state.ppc64.msr = regs.msr;
  state.ppc64.ctr = regs.ctr;
  state.ppc64.lr = regs.link;
  state.ppc64.xer = regs.xer;
  state.ppc64.cr = regs.ccr;
  state.ppc64.dar = regs.dar;
  state.ppc64.dsisr = regs.dsisr;

  // Read FPRs
  struct iovec iov;
  double fprs[32];
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRFPREG, &iov) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.ppc64.fp.regs[n] = fprs[n];
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
        state.ppc64.vr[n].v[i] = vrregs.vr[n][i];
      }
    }
    state.ppc64.vscr = vrregs.vscr;
    state.ppc64.vrsave = vrregs.vrsave;
  }

  // Read VSX registers if available (POWER7+)
  struct {
    uint64_t vsx[64][2];  // 64 x 128-bit VSX registers
  } vsxregs;

  iov.iov_base = &vsxregs;
  iov.iov_len = sizeof(vsxregs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PPC_VSX, &iov) == 0) {
    for (size_t n = 0; n < 64; n++) {
      state.ppc64.vsx[n].dw[0] = vsxregs.vsx[n][0];
      state.ppc64.vsx[n].dw[1] = vsxregs.vsx[n][1];
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Prepare GPRs and special registers
  struct ppc64_pt_regs regs;

  for (size_t n = 0; n < 32; n++) {
    regs.gpr[n] = state.ppc64.gp.regs[n];
  }

  regs.nip = state.ppc64.pc;
  regs.msr = state.ppc64.msr;
  regs.ctr = state.ppc64.ctr;
  regs.link = state.ppc64.lr;
  regs.xer = state.ppc64.xer;
  regs.ccr = state.ppc64.cr;
  regs.dar = state.ppc64.dar;
  regs.dsisr = state.ppc64.dsisr;
  regs.orig_gpr3 = 0;
  regs.softe = 0;
  regs.trap = 0;
  regs.result = 0;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &regs) < 0)
    return Platform::TranslateError();

  // Write FPRs
  struct iovec iov;
  double fprs[32];

  for (size_t n = 0; n < 32; n++) {
    fprs[n] = state.ppc64.fp.regs[n];
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
      vrregs.vr[n][i] = state.ppc64.vr[n].v[i];
    }
  }
  vrregs.vscr = state.ppc64.vscr;
  vrregs.vrsave = state.ppc64.vrsave;

  iov.iov_base = &vrregs;
  iov.iov_len = sizeof(vrregs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PPC_VMX, &iov) < 0) {
    // Non-fatal if AltiVec not available
  }

  // Write VSX registers if available (POWER7+)
  struct {
    uint64_t vsx[64][2];
  } vsxregs;

  for (size_t n = 0; n < 64; n++) {
    vsxregs.vsx[n][0] = state.ppc64.vsx[n].dw[0];
    vsxregs.vsx[n][1] = state.ppc64.vsx[n].dw[1];
  }

  iov.iov_base = &vsxregs;
  iov.iov_len = sizeof(vsxregs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PPC_VSX, &iov) < 0) {
    // Non-fatal if VSX not available
  }

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
