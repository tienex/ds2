//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SVR4_88/ProcFS.h"
#include "DebugServer2/Architecture/M88K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using ds2::Host::SVR4_88::ProcFS;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace SVR4_88 {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // System V/88 (AT&T System V Release 4 for m88k)
  // Uses procfs for debugging with SVR4 semantics
  // Access via /proc/<pid>/lwp/<lwpid> or /proc/<pid>
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d", ptid.pid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // System V/88 procfs provides prstatus structure
  // which contains the full register state
  //
  prstatus_t prstatus;
  if (::read(fd, &prstatus, sizeof(prstatus)) != sizeof(prstatus)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Extract general-purpose registers from prgregset_t
  // System V/88 uses standard m88k register layout
  //
  prgregset_t &gregs = prstatus.pr_reg;

  //
  // Copy m88k general-purpose registers (r0-r31)
  // Note: r0 is hardwired to zero in hardware
  //
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gregs[i];
  }

  // Enforce r0 = 0 (hardware constraint)
  state.gp.regs[0] = 0;

  //
  // Special registers
  // System V/88 prgregset_t layout:
  // [0-31]: r0-r31
  // [32]: SXIP (Shadow Execute IP)
  // [33]: SNIP (Shadow Next IP)
  // [34]: SFIP (Shadow Fetch IP)
  // [35]: SSBR (Shadow Scoreboard Register)
  // [36]: EPSR (Exception PSR)
  // [37]: FPSR (Floating-Point Status Register)
  // [38]: FPCR (Floating-Point Control Register)
  // [39]: FPECR (Floating-Point Exception Cause Register)
  //
  state.special.pc = gregs[32];     // SXIP
  state.special.npc = gregs[33];    // SNIP
  state.special.fpsr = gregs[37];   // FPSR
  state.special.fpcr = gregs[38];   // FPCR
  state.special.fpecr = gregs[39];  // FPECR

  //
  // Processor Status Register
  //
  std::memcpy(&state.psr, &gregs[36], sizeof(state.psr));

  //
  // Control registers
  //
  state.cr.cr1 = gregs[36];  // PSR (stored as EPSR)
  state.cr.cr2 = gregs[36];  // EPSR
  state.cr.cr3 = gregs[35];  // SSBR
  state.cr.cr4 = gregs[32];  // SXIP
  state.cr.cr5 = gregs[33];  // SNIP
  state.cr.cr6 = gregs[34];  // SFIP

  ::close(fd);

  //
  // Read floating-point registers via separate interface
  // System V/88 provides /proc/<pid>/fpregset
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregset", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    if (::read(fd, &fpregs, sizeof(fpregs)) == sizeof(fpregs)) {
      //
      // m88k FPU registers
      // System V/88 stores them as 32 x 32-bit values
      //
      for (int i = 0; i < 32; i++) {
        state.fpu.raw32[i] = fpregs.fp_r.fp_reg[i];
      }
    }
    ::close(fd);
  }

  //
  // Read additional control registers if available
  // System V/88 may provide extended procfs interface
  //
  snprintf(path, sizeof(path), "/proc/%d/ctlreg", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t vbr;      // CR7: Vector Base Register
      uint32_t dmt0;     // CR8
      uint32_t dmd0;     // CR9
      uint32_t dma0;     // CR10
      uint32_t dmt1;     // CR11
      uint32_t dmd1;     // CR12
      uint32_t dma1;     // CR13
      uint32_t dmt2;     // CR14
      uint32_t dmd2;     // CR15
      uint32_t dma2;     // CR16
      uint32_t sr0;      // CR17
      uint32_t sr1;      // CR18
      uint32_t sr2;      // CR19
      uint32_t sr3;      // CR20
    } ctlregs;

    if (::read(fd, &ctlregs, sizeof(ctlregs)) == sizeof(ctlregs)) {
      state.cr.cr7 = ctlregs.vbr;
      state.cr.cr8 = ctlregs.dmt0;
      state.cr.cr9 = ctlregs.dmd0;
      state.cr.cr10 = ctlregs.dma0;
      state.cr.cr11 = ctlregs.dmt1;
      state.cr.cr12 = ctlregs.dmd1;
      state.cr.cr13 = ctlregs.dma1;
      state.cr.cr14 = ctlregs.dmt2;
      state.cr.cr15 = ctlregs.dmd2;
      state.cr.cr16 = ctlregs.dma2;
      state.cr.cr17 = ctlregs.sr0;
      state.cr.cr18 = ctlregs.sr1;
      state.cr.cr19 = ctlregs.sr2;
      state.cr.cr20 = ctlregs.sr3;
    }
    ::close(fd);
  }

  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  //
  // System V/88 procfs write interface
  // Use /proc/<pid> with PIOCRUN ioctl or direct write
  //
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d", ptid.pid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // Construct prstatus structure with new register values
  //
  prstatus_t prstatus;
  std::memset(&prstatus, 0, sizeof(prstatus));

  prgregset_t &gregs = prstatus.pr_reg;

  //
  // Set general-purpose registers
  //
  for (int i = 0; i < 32; i++) {
    gregs[i] = state.gp.regs[i];
  }

  //
  // Set special registers
  //
  gregs[32] = state.special.pc;      // SXIP
  gregs[33] = state.special.npc;     // SNIP
  gregs[34] = state.cr.cr6;          // SFIP
  gregs[35] = state.cr.cr3;          // SSBR
  std::memcpy(&gregs[36], &state.psr, sizeof(uint32_t));  // EPSR
  gregs[37] = state.special.fpsr;    // FPSR
  gregs[38] = state.special.fpcr;    // FPCR
  gregs[39] = state.special.fpecr;   // FPECR

  //
  // Write back via PIOCRUN ioctl or direct write
  // System V/88 accepts direct writes to /proc/<pid>
  //
  if (::write(fd, &prstatus, sizeof(prstatus)) != sizeof(prstatus)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);

  //
  // Write floating-point registers
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregset", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    std::memset(&fpregs, 0, sizeof(fpregs));

    for (int i = 0; i < 32; i++) {
      fpregs.fp_r.fp_reg[i] = state.fpu.raw32[i];
    }

    ::write(fd, &fpregs, sizeof(fpregs));
    ::close(fd);
  }

  //
  // Write control registers if supported
  //
  snprintf(path, sizeof(path), "/proc/%d/ctlreg", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t vbr;
      uint32_t dmt0, dmd0, dma0;
      uint32_t dmt1, dmd1, dma1;
      uint32_t dmt2, dmd2, dma2;
      uint32_t sr0, sr1, sr2, sr3;
    } ctlregs;

    ctlregs.vbr = state.cr.cr7;
    ctlregs.dmt0 = state.cr.cr8;
    ctlregs.dmd0 = state.cr.cr9;
    ctlregs.dma0 = state.cr.cr10;
    ctlregs.dmt1 = state.cr.cr11;
    ctlregs.dmd1 = state.cr.cr12;
    ctlregs.dma1 = state.cr.cr13;
    ctlregs.dmt2 = state.cr.cr14;
    ctlregs.dmd2 = state.cr.cr15;
    ctlregs.dma2 = state.cr.cr16;
    ctlregs.sr0 = state.cr.cr17;
    ctlregs.sr1 = state.cr.cr18;
    ctlregs.sr2 = state.cr.cr19;
    ctlregs.sr3 = state.cr.cr20;

    // Ignore errors as control registers may be read-only
    ::write(fd, &ctlregs, sizeof(ctlregs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SVR4_88
} // namespace Host
} // namespace ds2
