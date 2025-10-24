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
#include "DebugServer2/Architecture/M68K/CPUState.h"
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
  // System V Release 4 m68k support
  // Used on various SVR4 m68k systems
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
  // SVR4 m68k prgregset_t layout:
  // [0-7]: d0-d7 (data registers)
  // [8-14]: a0-a6 (address registers)
  // [15]: usp (user stack pointer)
  // [16]: ssp (supervisor stack pointer)
  // [17]: pc (program counter)
  // [18]: sr (status register)
  //
  for (int i = 0; i < 8; i++) {
    state.data.regs[i] = gregs[i];
  }

  for (int i = 0; i < 7; i++) {
    state.addr.regs[i] = gregs[8 + i];
  }

  state.stack.usp = gregs[15];
  state.stack.ssp = gregs[16];
  state.special.pc = gregs[17];
  state.special.sr = gregs[18];

  //
  // Copy SR bit fields
  //
  std::memcpy(&state.sr_flags, &gregs[18], sizeof(state.sr_flags));

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
      // m68k FPU: 8 extended-precision (80-bit) registers
      //
      for (int i = 0; i < 8; i++) {
        state.fpu.fpr[i].mantissa = fpregs.fp_regs[i].mantissa;
        state.fpu.fpr[i].exponent = fpregs.fp_regs[i].exponent;
      }

      // FPU control registers
      state.fpu.fpcr = fpregs.fp_fpcr;
      state.fpu.fpsr = fpregs.fp_fpsr;
      state.fpu.fpiar = fpregs.fp_fpiar;
    }
    ::close(fd);
  }

  //
  // Read MMU registers if available
  // Some SVR4 variants provide /proc/<pid>/mmuregs
  //
  snprintf(path, sizeof(path), "/proc/%d/mmuregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t crp;
      uint32_t srp;
      uint16_t tc;
      uint16_t tt0;
      uint16_t tt1;
      uint32_t mmusr;
    } mmuregs;

    if (::read(fd, &mmuregs, sizeof(mmuregs)) == sizeof(mmuregs)) {
      state.mmu.crp = mmuregs.crp;
      state.mmu.srp = mmuregs.srp;
      state.mmu.tc = mmuregs.tc;
      state.mmu.tt0 = mmuregs.tt0;
      state.mmu.tt1 = mmuregs.tt1;
      state.mmu.mmusr = mmuregs.mmusr;
    }
    ::close(fd);
  }

  //
  // Read control registers if available
  //
  snprintf(path, sizeof(path), "/proc/%d/ctlregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t vbr;
      uint16_t sfc;
      uint16_t dfc;
      uint16_t cacr;
      uint32_t caar;
    } ctlregs;

    if (::read(fd, &ctlregs, sizeof(ctlregs)) == sizeof(ctlregs)) {
      state.control.vbr = ctlregs.vbr;
      state.control.sfc = ctlregs.sfc;
      state.control.dfc = ctlregs.dfc;
      state.cache.cacr = ctlregs.cacr;
      state.cache.caar = ctlregs.caar;
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

  for (int i = 0; i < 8; i++) {
    gregs[i] = state.data.regs[i];
  }

  for (int i = 0; i < 7; i++) {
    gregs[8 + i] = state.addr.regs[i];
  }

  gregs[15] = state.stack.usp;
  gregs[16] = state.stack.ssp;
  gregs[17] = state.special.pc;
  gregs[18] = state.special.sr;

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
      fpregs.fp_regs[i].mantissa = state.fpu.fpr[i].mantissa;
      fpregs.fp_regs[i].exponent = state.fpu.fpr[i].exponent;
    }

    fpregs.fp_fpcr = state.fpu.fpcr;
    fpregs.fp_fpsr = state.fpu.fpsr;
    fpregs.fp_fpiar = state.fpu.fpiar;

    ::write(fd, &fpregs, sizeof(fpregs));
    ::close(fd);
  }

  //
  // Write MMU registers if supported
  //
  snprintf(path, sizeof(path), "/proc/%d/mmuregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t crp;
      uint32_t srp;
      uint16_t tc;
      uint16_t tt0;
      uint16_t tt1;
      uint32_t mmusr;
    } mmuregs;

    mmuregs.crp = state.mmu.crp;
    mmuregs.srp = state.mmu.srp;
    mmuregs.tc = state.mmu.tc;
    mmuregs.tt0 = state.mmu.tt0;
    mmuregs.tt1 = state.mmu.tt1;
    mmuregs.mmusr = state.mmu.mmusr;

    ::write(fd, &mmuregs, sizeof(mmuregs));
    ::close(fd);
  }

  //
  // Write control registers if supported
  //
  snprintf(path, sizeof(path), "/proc/%d/ctlregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t vbr;
      uint16_t sfc;
      uint16_t dfc;
      uint16_t cacr;
      uint32_t caar;
    } ctlregs;

    ctlregs.vbr = state.control.vbr;
    ctlregs.sfc = state.control.sfc;
    ctlregs.dfc = state.control.dfc;
    ctlregs.cacr = state.cache.cacr;
    ctlregs.caar = state.cache.caar;

    ::write(fd, &ctlregs, sizeof(ctlregs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SVR4
} // namespace Host
} // namespace ds2
