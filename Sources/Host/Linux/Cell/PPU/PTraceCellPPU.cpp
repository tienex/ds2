//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PlayStation 3 Cell Broadband Engine - PPU (PowerPC Processing Element)
// The PPU is a 64-bit PowerPC with AltiVec/VMX
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

// Cell PPU ptrace register layout (similar to PowerPC64)
struct cell_ppu_pt_regs {
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
  struct cell_ppu_pt_regs regs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &regs) < 0)
    return Platform::TranslateError();

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    state.cell_ppu.gp.regs[n] = regs.gpr[n];
  }

  // Copy special registers
  state.cell_ppu.pc = regs.nip;
  state.cell_ppu.msr = regs.msr;
  state.cell_ppu.ctr = regs.ctr;
  state.cell_ppu.lr = regs.link;
  state.cell_ppu.xer = regs.xer;
  state.cell_ppu.cr = regs.ccr;
  state.cell_ppu.dar = regs.dar;
  state.cell_ppu.dsisr = regs.dsisr;

  // Read FPRs
  struct iovec iov;
  double fprs[32];
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRFPREG, &iov) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.cell_ppu.fp.regs[n] = fprs[n];
    }
  }

  // Read AltiVec/VMX registers (Cell PPU has standard 32 vector registers)
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
        state.cell_ppu.vr[n].v[i] = vrregs.vr[n][i];
      }
    }
    state.cell_ppu.vscr = vrregs.vscr;
    state.cell_ppu.vrsave = vrregs.vrsave;
  }

  // Cell-specific: Read SPU mailbox status if available
  // The PPU can communicate with SPUs via mailboxes
  // This would require additional Cell-specific ptrace extensions

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Prepare GPRs and special registers
  struct cell_ppu_pt_regs regs;

  for (size_t n = 0; n < 32; n++) {
    regs.gpr[n] = state.cell_ppu.gp.regs[n];
  }

  regs.nip = state.cell_ppu.pc;
  regs.msr = state.cell_ppu.msr;
  regs.ctr = state.cell_ppu.ctr;
  regs.link = state.cell_ppu.lr;
  regs.xer = state.cell_ppu.xer;
  regs.ccr = state.cell_ppu.cr;
  regs.dar = state.cell_ppu.dar;
  regs.dsisr = state.cell_ppu.dsisr;
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
    fprs[n] = state.cell_ppu.fp.regs[n];
  }

  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRFPREG, &iov) < 0) {
    // Non-fatal if FPU not available
  }

  // Write AltiVec/VMX registers
  struct {
    uint32_t vr[32][4];
    uint32_t vscr;
    uint32_t vrsave;
  } vrregs;

  for (size_t n = 0; n < 32; n++) {
    for (size_t i = 0; i < 4; i++) {
      vrregs.vr[n][i] = state.cell_ppu.vr[n].v[i];
    }
  }
  vrregs.vscr = state.cell_ppu.vscr;
  vrregs.vrsave = state.cell_ppu.vrsave;

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
