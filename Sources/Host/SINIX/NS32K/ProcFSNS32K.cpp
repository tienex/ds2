//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SINIX/ProcFS.h"
#include "DebugServer2/Architecture/NS32K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using ds2::Host::SINIX::ProcFS;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace SINIX {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // SINIX (Siemens/SNI Unix) NS32K support
  // Uses procfs with ELF binary format
  // Access via /proc/<pid>
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
  // SINIX NS32K prgregset_t layout:
  // [0-7]: r0-r7 general registers
  // [8]: fp (frame pointer)
  // [9]: sp (stack pointer)
  // [10]: sb (static base)
  // [11]: pc (program counter)
  // [12]: psr (processor status register)
  // [13]: mod (module register)
  //
  for (int i = 0; i < 8; i++) {
    state.gp.regs[i] = gregs[i];
  }

  state.special.fp = gregs[8];
  state.special.sp = gregs[9];
  state.special.sb = gregs[10];
  state.special.pc = gregs[11];
  state.special.psr = gregs[12];
  state.special.mod = gregs[13];

  //
  // Copy PSR bit fields
  //
  std::memcpy(&state.psr_flags, &gregs[12], sizeof(state.psr_flags));

  ::close(fd);

  //
  // Read floating-point registers
  // SINIX provides /proc/<pid>/fpregs
  //
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    prfpregset_t fpregs;
    if (::read(fd, &fpregs, sizeof(fpregs)) == sizeof(fpregs)) {
      //
      // NS32K FPU: 8 single-precision registers
      //
      for (int i = 0; i < 8; i++) {
        state.fpu.raw32[i] = fpregs.fp_reg[i];
      }

      // FPU status register
      state.fsr.rm = fpregs.fp_fsr & 0x3;
      state.fsr.uf = (fpregs.fp_fsr >> 5) & 1;
      state.fsr.if_ = (fpregs.fp_fsr >> 6) & 1;
      state.fsr.tt = (fpregs.fp_fsr >> 7) & 1;
    }
    ::close(fd);
  }

  //
  // Read MMU registers if available
  // SINIX may provide /proc/<pid>/mmuregs for NS32532
  //
  snprintf(path, sizeof(path), "/proc/%d/mmuregs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t ptb0, ptb1, eia, mcr, msr, tear;
    } mmuregs;

    if (::read(fd, &mmuregs, sizeof(mmuregs)) == sizeof(mmuregs)) {
      state.mmu.ptb0 = mmuregs.ptb0;
      state.mmu.ptb1 = mmuregs.ptb1;
      state.mmu.eia = mmuregs.eia;
      state.mmu.mcr = mmuregs.mcr;
      state.mmu.msr = mmuregs.msr;
      state.mmu.tear = mmuregs.tear;
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
    gregs[i] = state.gp.regs[i];
  }

  gregs[8] = state.special.fp;
  gregs[9] = state.special.sp;
  gregs[10] = state.special.sb;
  gregs[11] = state.special.pc;
  gregs[12] = state.special.psr;
  gregs[13] = state.special.mod;

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
      fpregs.fp_reg[i] = state.fpu.raw32[i];
    }

    // Encode FSR
    fpregs.fp_fsr = (state.fsr.rm & 0x3) |
                    ((state.fsr.uf & 1) << 5) |
                    ((state.fsr.if_ & 1) << 6) |
                    ((state.fsr.tt & 1) << 7);

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
      uint32_t ptb0, ptb1, eia, mcr, msr, tear;
    } mmuregs;

    mmuregs.ptb0 = state.mmu.ptb0;
    mmuregs.ptb1 = state.mmu.ptb1;
    mmuregs.eia = state.mmu.eia;
    mmuregs.mcr = state.mmu.mcr;
    mmuregs.msr = state.mmu.msr;
    mmuregs.tear = state.mmu.tear;

    ::write(fd, &mmuregs, sizeof(mmuregs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SINIX
} // namespace Host
} // namespace ds2
