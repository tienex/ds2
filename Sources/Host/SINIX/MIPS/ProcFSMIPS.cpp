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
#include "DebugServer2/Architecture/MIPS/CPUState.h"
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
  // SINIX MIPS support (Siemens/SNI Unix on MIPS R3000/R4000)
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
  // SINIX MIPS prgregset_t layout (SVR4 style):
  // [0-31]: General registers $0-$31
  // [32]: lo
  // [33]: hi
  // [34]: pc
  // [35]: cause
  // [36]: badvaddr
  //
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gregs[i];
  }

  // Enforce $zero = 0 (hardware constraint)
  state.gp.regs[0] = 0;

  //
  // Special registers
  //
  state.special.lo = gregs[32];
  state.special.hi = gregs[33];
  state.special.pc = gregs[34];

  //
  // CP0 registers
  //
  state.cp0.cause = gregs[35];
  state.cp0.badvaddr = gregs[36];

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
      // MIPS FPU registers (32 single-precision)
      //
      for (int i = 0; i < 32; i++) {
        state.fpu.fpr[i] = fpregs.fp_r.fp_regs[i];
      }

      // FPU Control/Status Register
      state.fpu.fcsr = fpregs.fp_csr;
      state.fpu.fir = fpregs.fp_fir;
    }
    ::close(fd);
  }

  //
  // Read CP0 registers if available
  // SINIX may provide extended interface
  //
  snprintf(path, sizeof(path), "/proc/%d/cp0regs", ptid.pid);
  fd = ::open(path, O_RDONLY);
  if (fd >= 0) {
    struct {
      uint32_t index, random, entrylo0, entrylo1;
      uint32_t context, pagemask, wired, reserved1;
      uint32_t badvaddr, count, entryhi, compare;
      uint32_t status, cause, epc, prid;
      uint32_t config, lladdr, watchlo, watchhi;
    } cp0regs;

    if (::read(fd, &cp0regs, sizeof(cp0regs)) == sizeof(cp0regs)) {
      state.cp0.index = cp0regs.index;
      state.cp0.random = cp0regs.random;
      state.cp0.entrylo0 = cp0regs.entrylo0;
      state.cp0.entrylo1 = cp0regs.entrylo1;
      state.cp0.context = cp0regs.context;
      state.cp0.pagemask = cp0regs.pagemask;
      state.cp0.wired = cp0regs.wired;
      state.cp0.badvaddr = cp0regs.badvaddr;
      state.cp0.count = cp0regs.count;
      state.cp0.entryhi = cp0regs.entryhi;
      state.cp0.compare = cp0regs.compare;
      state.cp0.status = cp0regs.status;
      state.cp0.cause = cp0regs.cause;
      state.cp0.epc = cp0regs.epc;
      state.cp0.prid = cp0regs.prid;
      state.cp0.config = cp0regs.config;
      state.cp0.lladdr = cp0regs.lladdr;
      state.cp0.watchlo = cp0regs.watchlo;
      state.cp0.watchhi = cp0regs.watchhi;
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

  for (int i = 0; i < 32; i++) {
    gregs[i] = state.gp.regs[i];
  }

  gregs[32] = state.special.lo;
  gregs[33] = state.special.hi;
  gregs[34] = state.special.pc;
  gregs[35] = state.cp0.cause;
  gregs[36] = state.cp0.badvaddr;

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

    for (int i = 0; i < 32; i++) {
      fpregs.fp_r.fp_regs[i] = state.fpu.fpr[i];
    }

    fpregs.fp_csr = state.fpu.fcsr;
    fpregs.fp_fir = state.fpu.fir;

    ::write(fd, &fpregs, sizeof(fpregs));
    ::close(fd);
  }

  //
  // Write CP0 registers if supported
  //
  snprintf(path, sizeof(path), "/proc/%d/cp0regs", ptid.pid);
  fd = ::open(path, O_WRONLY);
  if (fd >= 0) {
    struct {
      uint32_t index, random, entrylo0, entrylo1;
      uint32_t context, pagemask, wired, reserved1;
      uint32_t badvaddr, count, entryhi, compare;
      uint32_t status, cause, epc, prid;
      uint32_t config, lladdr, watchlo, watchhi;
    } cp0regs;

    cp0regs.index = state.cp0.index;
    cp0regs.random = state.cp0.random;
    cp0regs.entrylo0 = state.cp0.entrylo0;
    cp0regs.entrylo1 = state.cp0.entrylo1;
    cp0regs.context = state.cp0.context;
    cp0regs.pagemask = state.cp0.pagemask;
    cp0regs.wired = state.cp0.wired;
    cp0regs.badvaddr = state.cp0.badvaddr;
    cp0regs.count = state.cp0.count;
    cp0regs.entryhi = state.cp0.entryhi;
    cp0regs.compare = state.cp0.compare;
    cp0regs.status = state.cp0.status;
    cp0regs.cause = state.cp0.cause;
    cp0regs.epc = state.cp0.epc;
    cp0regs.prid = state.cp0.prid;
    cp0regs.config = state.cp0.config;
    cp0regs.lladdr = state.cp0.lladdr;
    cp0regs.watchlo = state.cp0.watchlo;
    cp0regs.watchhi = state.cp0.watchhi;

    ::write(fd, &cp0regs, sizeof(cp0regs));
    ::close(fd);
  }

  return kSuccess;
}

} // namespace SINIX
} // namespace Host
} // namespace ds2
