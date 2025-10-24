//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SVR4/ProcFS.h"
#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using ds2::Host::SVR4::ProcFS;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace SVR4 {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // Generic System V Release 4 x86 support
  // Includes: UnixWare 1.x/2.x/7.x, UnixWare 4.0MP, UnixWare 4.2
  // SCO OpenServer 5.x/6.x, Solaris x86, other SVR4 variants
  // Uses procfs for debugging
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d", ptid.pid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // Read prstatus structure containing register state
  //
  prstatus_t prstatus;
  if (::read(fd, &prstatus, sizeof(prstatus)) != sizeof(prstatus)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  prgregset_t &gregs = prstatus.pr_reg;

  //
  // SVR4 x86 prgregset_t layout (standard across all SVR4 variants):
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

  ::close(fd);

  //
  // Read floating-point registers
  // SVR4 provides /proc/<pid>/fpregs for FPU state
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    if (::read(fd, &fpregs, sizeof(fpregs)) == sizeof(fpregs)) {
      //
      // x87 FPU registers (80-bit extended precision)
      //
      for (int i = 0; i < 8; i++) {
        std::memcpy(&state.x87.fpregs[i],
                    &fpregs.fp_reg_set.fpchip_state.st[i],
                    sizeof(state.x87.fpregs[i]));
      }

      state.x87.fctrl = fpregs.fp_reg_set.fpchip_state.cw;
      state.x87.fstat = fpregs.fp_reg_set.fpchip_state.sw;
      state.x87.ftag = fpregs.fp_reg_set.fpchip_state.tag;
      state.x87.fop = 0;  // Not always available
      state.x87.fioff = fpregs.fp_reg_set.fpchip_state.ipoff;
      state.x87.fiseg = fpregs.fp_reg_set.fpchip_state.cssel;
      state.x87.fooff = fpregs.fp_reg_set.fpchip_state.dataoff;
      state.x87.foseg = fpregs.fp_reg_set.fpchip_state.datasel;
    }
    ::close(fd);
  }

  //
  // Read debug registers if available (UnixWare 4.x+)
  // Some SVR4 variants provide /proc/<pid>/dbregs
  //
  snprintf(path, sizeof(path), "/proc/%d/dbregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t dr0, dr1, dr2, dr3;
      uint32_t dr4, dr5, dr6, dr7;
    } dbregs;

    if (::read(fd, &dbregs, sizeof(dbregs)) == sizeof(dbregs)) {
      state.debug.dr0 = dbregs.dr0;
      state.debug.dr1 = dbregs.dr1;
      state.debug.dr2 = dbregs.dr2;
      state.debug.dr3 = dbregs.dr3;
      state.debug.dr6 = dbregs.dr6;
      state.debug.dr7 = dbregs.dr7;
    }
    ::close(fd);
  }

  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  //
  // Write register state via procfs
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d", ptid.pid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // Construct prstatus with new register values
  //
  prstatus_t prstatus;
  std::memset(&prstatus, 0, sizeof(prstatus));

  prgregset_t &gregs = prstatus.pr_reg;

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

  if (::write(fd, &prstatus, sizeof(prstatus)) != sizeof(prstatus)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);

  //
  // Write floating-point registers
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
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

    ::write(fd, &fpregs, sizeof(fpregs));
    ::close(fd);
  }

  //
  // Write debug registers if supported
  //
  snprintf(path, sizeof(path), "/proc/%d/dbregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t dr0, dr1, dr2, dr3;
      uint32_t dr4, dr5, dr6, dr7;
    } dbregs;

    dbregs.dr0 = state.debug.dr0;
    dbregs.dr1 = state.debug.dr1;
    dbregs.dr2 = state.debug.dr2;
    dbregs.dr3 = state.debug.dr3;
    dbregs.dr6 = state.debug.dr6;
    dbregs.dr7 = state.debug.dr7;

    ::write(fd, &dbregs, sizeof(dbregs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SVR4
} // namespace Host
} // namespace ds2
