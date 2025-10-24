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
#include "DebugServer2/Architecture/WE32K/CPUState.h"
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
  // AT&T UNIX System V Release 4 for WE32K
  // Uses procfs for debugging
  // Primary platforms: AT&T 3B2, 3B5, 3B15, 3B20
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
  // AT&T SVR4 WE32K prgregset_t layout:
  // [0-8]: r0-r8 (general purpose registers)
  // [9]: fp (Frame Pointer, R9)
  // [10]: ap (Argument Pointer, R10)
  // [11]: psw (Processor Status Word, R11)
  // [12]: sp (Stack Pointer, R12)
  // [13]: pcbp (Process Control Block Pointer, R13)
  // [14]: isp (Interrupt Stack Pointer, R14)
  // [15]: pc (Program Counter, R15)
  //
  for (int i = 0; i < 9; i++) {
    state.gp.regs[i] = gregs[i];
  }

  //
  // Special purpose registers
  //
  state.special.fp = gregs[9];
  state.special.ap = gregs[10];
  state.special.psw = gregs[11];
  state.special.sp = gregs[12];
  state.special.pcbp = gregs[13];
  state.special.isp = gregs[14];
  state.special.pc = gregs[15];

  //
  // Copy PSW bit fields
  //
  std::memcpy(&state.psw_flags, &gregs[11], sizeof(state.psw_flags));

  ::close(fd);

  //
  // Read MAU (Math Accelerator Unit) registers if present
  // SVR4 provides /proc/<pid>/fpregs for FPU state
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    if (::read(fd, &fpregs, sizeof(fpregs)) == sizeof(fpregs)) {
      //
      // WE32K MAU: 4 double-precision registers
      // SVR4 stores them as raw 64-bit values
      //
      for (int i = 0; i < 4; i++) {
        state.mau.regs.raw64[i] = fpregs.fp_reg[i];
      }

      // MAU Status Register (MASR)
      std::memcpy(&state.mau.masr, &fpregs.fp_masr, sizeof(state.mau.masr));

      // MAU Control Register (MACR)
      state.mau.macr = fpregs.fp_macr;
    }
    ::close(fd);
  }

  //
  // Read MMU registers if available
  // SVR4 may provide extended interface for MMU state
  //
  snprintf(path, sizeof(path), "/proc/%d/mmuregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t srama[64];
      uint32_t sramb[64];
      uint32_t fltcr;
      uint32_t fltadr;
      uint32_t sdr[3];
    } mmuregs;

    if (::read(fd, &mmuregs, sizeof(mmuregs)) == sizeof(mmuregs)) {
      std::memcpy(state.mmu.srama, mmuregs.srama, sizeof(state.mmu.srama));
      std::memcpy(state.mmu.sramb, mmuregs.sramb, sizeof(state.mmu.sramb));
      state.mmu.fltcr = mmuregs.fltcr;
      state.mmu.fltadr = mmuregs.fltadr;
      std::memcpy(state.mmu.sdr, mmuregs.sdr, sizeof(state.mmu.sdr));
    }
    ::close(fd);
  }

  //
  // Read system control registers
  // SVR4 may expose via /proc/<pid>/sysctlregs
  //
  snprintf(path, sizeof(path), "/proc/%d/sysctlregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t ivtp;
      uint32_t acr;
      uint32_t asr;
      uint32_t uar;
      uint32_t tcr;
    } sysregs;

    if (::read(fd, &sysregs, sizeof(sysregs)) == sizeof(sysregs)) {
      state.system.ivtp = sysregs.ivtp;
      state.system.acr = sysregs.acr;
      state.system.asr = sysregs.asr;
      state.system.uar = sysregs.uar;
      state.system.tcr = sysregs.tcr;
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

  //
  // Set general purpose registers
  //
  for (int i = 0; i < 9; i++) {
    gregs[i] = state.gp.regs[i];
  }

  //
  // Set special purpose registers
  //
  gregs[9] = state.special.fp;
  gregs[10] = state.special.ap;
  gregs[11] = state.special.psw;
  gregs[12] = state.special.sp;
  gregs[13] = state.special.pcbp;
  gregs[14] = state.special.isp;
  gregs[15] = state.special.pc;

  if (::write(fd, &prstatus, sizeof(prstatus)) != sizeof(prstatus)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);

  //
  // Write MAU registers if present
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    std::memset(&fpregs, 0, sizeof(fpregs));

    for (int i = 0; i < 4; i++) {
      fpregs.fp_reg[i] = state.mau.regs.raw64[i];
    }

    std::memcpy(&fpregs.fp_masr, &state.mau.masr, sizeof(fpregs.fp_masr));
    fpregs.fp_macr = state.mau.macr;

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
      uint32_t srama[64];
      uint32_t sramb[64];
      uint32_t fltcr;
      uint32_t fltadr;
      uint32_t sdr[3];
    } mmuregs;

    std::memcpy(mmuregs.srama, state.mmu.srama, sizeof(mmuregs.srama));
    std::memcpy(mmuregs.sramb, state.mmu.sramb, sizeof(mmuregs.sramb));
    mmuregs.fltcr = state.mmu.fltcr;
    mmuregs.fltadr = state.mmu.fltadr;
    std::memcpy(mmuregs.sdr, state.mmu.sdr, sizeof(mmuregs.sdr));

    ::write(fd, &mmuregs, sizeof(mmuregs));
    ::close(fd);
  }

  //
  // Write system control registers
  //
  snprintf(path, sizeof(path), "/proc/%d/sysctlregs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t ivtp;
      uint32_t acr;
      uint32_t asr;
      uint32_t uar;
      uint32_t tcr;
    } sysregs;

    sysregs.ivtp = state.system.ivtp;
    sysregs.acr = state.system.acr;
    sysregs.asr = state.system.asr;
    sysregs.uar = state.system.uar;
    sysregs.tcr = state.system.tcr;

    ::write(fd, &sysregs, sizeof(sysregs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SVR4
} // namespace Host
} // namespace ds2
