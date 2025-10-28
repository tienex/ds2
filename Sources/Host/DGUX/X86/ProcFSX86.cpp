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
#include "DebugServer2/Architecture/X86/CPUState.h"
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
  // DG/UX uses procfs for debugging
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
  // DG/UX procfs uses prgregset_t (System V style)
  //
  prgregset_t gregs;
  if (::pread(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Map System V x86 register layout to our CPUState
  // DG/UX follows SVR4 x86 ABI register ordering:
  // GS, FS, ES, DS, EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX,
  // TRAPNO, ERR, EIP, CS, EFL, UESP, SS
  //
  state.gp.eax = gregs[EAX];
  state.gp.ebx = gregs[EBX];
  state.gp.ecx = gregs[ECX];
  state.gp.edx = gregs[EDX];
  state.gp.esi = gregs[ESI];
  state.gp.edi = gregs[EDI];
  state.gp.ebp = gregs[EBP];
  state.gp.esp = gregs[UESP];  // User ESP
  state.gp.eip = gregs[EIP];
  state.gp.eflags = gregs[EFL];

  //
  // Segment registers
  //
  state.segments.cs = gregs[CS];
  state.segments.ss = gregs[SS];
  state.segments.ds = gregs[DS];
  state.segments.es = gregs[ES];
  state.segments.fs = gregs[FS];
  state.segments.gs = gregs[GS];

  //
  // Read floating-point registers
  // DG/UX provides FPU state via prfpregset_t
  //
  prfpregset_t fpregs;
  off_t fp_offset = sizeof(gregs);

  if (::pread(fd, &fpregs, sizeof(fpregs), fp_offset) == sizeof(fpregs)) {
    //
    // x87 FPU registers
    // DG/UX uses the standard x87 layout
    //
    for (int i = 0; i < 8; i++) {
      // 80-bit extended precision format
      std::memcpy(&state.x87.fpregs[i], &fpregs.fp_reg_set.fpchip_state.st[i],
                  sizeof(state.x87.fpregs[i]));
    }

    state.x87.fctrl = fpregs.fp_reg_set.fpchip_state.cw;
    state.x87.fstat = fpregs.fp_reg_set.fpchip_state.sw;
    state.x87.ftag = fpregs.fp_reg_set.fpchip_state.tag;
    state.x87.fop = 0;  // Not available in basic procfs
    state.x87.fioff = fpregs.fp_reg_set.fpchip_state.ipoff;
    state.x87.fiseg = fpregs.fp_reg_set.fpchip_state.cssel;
    state.x87.fooff = fpregs.fp_reg_set.fpchip_state.dataoff;
    state.x87.foseg = fpregs.fp_reg_set.fpchip_state.datasel;
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
  gregs[EAX] = state.gp.eax;
  gregs[EBX] = state.gp.ebx;
  gregs[ECX] = state.gp.ecx;
  gregs[EDX] = state.gp.edx;
  gregs[ESI] = state.gp.esi;
  gregs[EDI] = state.gp.edi;
  gregs[EBP] = state.gp.ebp;
  gregs[UESP] = state.gp.esp;
  gregs[EIP] = state.gp.eip;
  gregs[EFL] = state.gp.eflags;
  gregs[CS] = state.segments.cs;
  gregs[SS] = state.segments.ss;
  gregs[DS] = state.segments.ds;
  gregs[ES] = state.segments.es;
  gregs[FS] = state.segments.fs;
  gregs[GS] = state.segments.gs;

  if (::pwrite(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
  prfpregset_t fpregs;
  std::memset(&fpregs, 0, sizeof(fpregs));

  for (int i = 0; i < 8; i++) {
    std::memcpy(&fpregs.fp_reg_set.fpchip_state.st[i],
                &state.x87.fpregs[i],
                sizeof(state.x87.fpregs[i]));
  }

  fpregs.fp_reg_set.fpchip_state.cw = state.x87.fctrl;
  fpregs.fp_reg_set.fpchip_state.sw = state.x87.fstat;
  fpregs.fp_reg_set.fpchip_state.tag = state.x87.ftag;
  fpregs.fp_reg_set.fpchip_state.ipoff = state.x87.fioff;
  fpregs.fp_reg_set.fpchip_state.cssel = state.x87.fiseg;
  fpregs.fp_reg_set.fpchip_state.dataoff = state.x87.fooff;
  fpregs.fp_reg_set.fpchip_state.datasel = state.x87.foseg;

  off_t fp_offset = sizeof(gregs);
  if (::pwrite(fd, &fpregs, sizeof(fpregs), fp_offset) != sizeof(fpregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);
  return kSuccess;
}

} // namespace DGUX
} // namespace Host
} // namespace ds2
