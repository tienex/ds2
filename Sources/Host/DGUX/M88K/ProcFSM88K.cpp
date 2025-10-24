//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/DGUX/ProcFS.h"
#include "DebugServer2/Architecture/M88K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using ds2::Host::DGUX::ProcFS;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace DGUX {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // DG/UX m88k uses procfs for debugging
  // Access via /proc/<pid>/lwp/<lwpid>/lwpctl
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", ptid.pid, ptid.tid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0) {
    // Try older DG/UX procfs layout: /proc/<pid>/ctl
    snprintf(path, sizeof(path), "/proc/%d/ctl", ptid.pid);
    fd = ::open(path, O_RDONLY);
    if (fd < 0) {
      return Platform::TranslateError();
    }
  }

  //
  // Read general-purpose registers
  // DG/UX m88k procfs provides prgregset_t with m88k register layout
  //
  prgregset_t gregs;
  if (::pread(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Copy m88k general-purpose registers (r0-r31)
  // Note: r0 is hardwired to zero in hardware
  //
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gregs.r_r[i];
  }

  // Force r0 to zero (hardware constraint)
  state.gp.regs[0] = 0;

  //
  // Special registers
  //
  state.special.pc = gregs.r_pc;
  state.special.npc = gregs.r_npc;
  state.special.fpecr = gregs.r_fpecr;
  state.special.fpcr = gregs.r_fpcr;
  state.special.fpsr = gregs.r_fpsr;

  //
  // Processor Status Register (PSR)
  //
  std::memcpy(&state.psr, &gregs.r_psr, sizeof(state.psr));

  //
  // Read floating-point registers
  // DG/UX provides m88k FPU state
  //
  prfpregset_t fpregs;
  off_t fp_offset = sizeof(gregs);

  if (::pread(fd, &fpregs, sizeof(fpregs), fp_offset) == sizeof(fpregs)) {
    //
    // m88k FPU registers
    // Can be accessed as 32 single-precision or 16 double-precision
    //
    for (int i = 0; i < 32; i++) {
      state.fpu.raw32[i] = fpregs.fp_regs[i];
    }
  }

  //
  // Read control registers (supervisor mode)
  // Available via extended procfs interface
  //
  off_t cr_offset = sizeof(gregs) + sizeof(fpregs);
  struct cr_regs {
    uint32_t cr[21];  // CR0-CR20
  } cr_regs;

  if (::pread(fd, &cr_regs, sizeof(cr_regs), cr_offset) == sizeof(cr_regs)) {
    state.cr.cr0 = cr_regs.cr[0];   // PID
    state.cr.cr1 = cr_regs.cr[1];   // PSR
    state.cr.cr2 = cr_regs.cr[2];   // EPSR
    state.cr.cr3 = cr_regs.cr[3];   // SSBR
    state.cr.cr4 = cr_regs.cr[4];   // SXIP
    state.cr.cr5 = cr_regs.cr[5];   // SNIP
    state.cr.cr6 = cr_regs.cr[6];   // SFIP
    state.cr.cr7 = cr_regs.cr[7];   // VBR
    state.cr.cr8 = cr_regs.cr[8];   // DMT0
    state.cr.cr9 = cr_regs.cr[9];   // DMD0
    state.cr.cr10 = cr_regs.cr[10]; // DMA0
    state.cr.cr11 = cr_regs.cr[11]; // DMT1
    state.cr.cr12 = cr_regs.cr[12]; // DMD1
    state.cr.cr13 = cr_regs.cr[13]; // DMA1
    state.cr.cr14 = cr_regs.cr[14]; // DMT2
    state.cr.cr15 = cr_regs.cr[15]; // DMD2
    state.cr.cr16 = cr_regs.cr[16]; // DMA2
    state.cr.cr17 = cr_regs.cr[17]; // SR0
    state.cr.cr18 = cr_regs.cr[18]; // SR1
    state.cr.cr19 = cr_regs.cr[19]; // SR2
    state.cr.cr20 = cr_regs.cr[20]; // SR3
  }

  ::close(fd);
  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  //
  // Open procfs control file for writing
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", ptid.pid, ptid.tid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0) {
    // Try older procfs layout
    snprintf(path, sizeof(path), "/proc/%d/ctl", ptid.pid);
    fd = ::open(path, O_WRONLY);
    if (fd < 0) {
      return Platform::TranslateError();
    }
  }

  //
  // Write general-purpose registers
  //
  prgregset_t gregs;
  for (int i = 0; i < 32; i++) {
    // r0 is hardwired to zero, but we write it anyway
    gregs.r_r[i] = state.gp.regs[i];
  }

  gregs.r_pc = state.special.pc;
  gregs.r_npc = state.special.npc;
  gregs.r_fpecr = state.special.fpecr;
  gregs.r_fpcr = state.special.fpcr;
  gregs.r_fpsr = state.special.fpsr;
  std::memcpy(&gregs.r_psr, &state.psr, sizeof(gregs.r_psr));

  if (::pwrite(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
  prfpregset_t fpregs;
  for (int i = 0; i < 32; i++) {
    fpregs.fp_regs[i] = state.fpu.raw32[i];
  }

  off_t fp_offset = sizeof(gregs);
  if (::pwrite(fd, &fpregs, sizeof(fpregs), fp_offset) != sizeof(fpregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Write control registers (if permitted)
  //
  struct cr_regs {
    uint32_t cr[21];
  } cr_regs;

  cr_regs.cr[0] = state.cr.cr0;
  cr_regs.cr[1] = state.cr.cr1;
  cr_regs.cr[2] = state.cr.cr2;
  cr_regs.cr[3] = state.cr.cr3;
  cr_regs.cr[4] = state.cr.cr4;
  cr_regs.cr[5] = state.cr.cr5;
  cr_regs.cr[6] = state.cr.cr6;
  cr_regs.cr[7] = state.cr.cr7;
  cr_regs.cr[8] = state.cr.cr8;
  cr_regs.cr[9] = state.cr.cr9;
  cr_regs.cr[10] = state.cr.cr10;
  cr_regs.cr[11] = state.cr.cr11;
  cr_regs.cr[12] = state.cr.cr12;
  cr_regs.cr[13] = state.cr.cr13;
  cr_regs.cr[14] = state.cr.cr14;
  cr_regs.cr[15] = state.cr.cr15;
  cr_regs.cr[16] = state.cr.cr16;
  cr_regs.cr[17] = state.cr.cr17;
  cr_regs.cr[18] = state.cr.cr18;
  cr_regs.cr[19] = state.cr.cr19;
  cr_regs.cr[20] = state.cr.cr20;

  off_t cr_offset = sizeof(gregs) + sizeof(fpregs);
  // Ignore errors for control registers as they may be read-only
  ::pwrite(fd, &cr_regs, sizeof(cr_regs), cr_offset);

  ::close(fd);
  return kSuccess;
}

} // namespace DGUX
} // namespace Host
} // namespace ds2
