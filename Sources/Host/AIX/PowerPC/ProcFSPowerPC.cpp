//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SystemV/ProcFS.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/procfs.h>
#include <sys/ptrace.h>

namespace ds2 {
namespace Host {
namespace SystemV {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  // AIX uses /proc/<pid>/status and /proc/<pid>/regs
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/regs", ptid.pid);

  int fd = open(path, O_RDONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // AIX procfs register structure for PowerPC
  struct prgregset {
    uint32_t gpr[32];
    uint32_t msr;
    uint32_t iar;  // Instruction Address Register (PC)
    uint32_t lr;
    uint32_t ctr;
    uint32_t cr;
    uint32_t xer;
    uint32_t fpscr;
    uint32_t fpscrx;
  } regs;

  ssize_t nread = read(fd, &regs, sizeof(regs));
  close(fd);

  if (nread < 0)
    return Platform::TranslateError();

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = regs.gpr[n];
  }

  // Copy special registers
  state.ppc.pc = regs.iar;
  state.ppc.msr = regs.msr;
  state.ppc.lr = regs.lr;
  state.ppc.ctr = regs.ctr;
  state.ppc.cr = regs.cr;
  state.ppc.xer = regs.xer;
  state.ppc.fpscr = regs.fpscr;

  // Read floating-point registers from separate file
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = open(path, O_RDONLY);
  if (fd >= 0) {
    double fprs[32];
    nread = read(fd, &fprs, sizeof(fprs));
    close(fd);

    if (nread > 0) {
      for (size_t n = 0; n < 32; n++) {
        state.ppc.fp.regs[n] = fprs[n];
      }
    }
  }

  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  // AIX uses /proc/<pid>/ctl for control operations
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/regs", ptid.pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Prepare AIX procfs register structure
  struct prgregset {
    uint32_t gpr[32];
    uint32_t msr;
    uint32_t iar;
    uint32_t lr;
    uint32_t ctr;
    uint32_t cr;
    uint32_t xer;
    uint32_t fpscr;
    uint32_t fpscrx;
  } regs;

  for (size_t n = 0; n < 32; n++) {
    regs.gpr[n] = state.ppc.gp.regs[n];
  }

  regs.iar = state.ppc.pc;
  regs.msr = state.ppc.msr;
  regs.lr = state.ppc.lr;
  regs.ctr = state.ppc.ctr;
  regs.cr = state.ppc.cr;
  regs.xer = state.ppc.xer;
  regs.fpscr = state.ppc.fpscr;
  regs.fpscrx = 0;

  ssize_t nwritten = write(fd, &regs, sizeof(regs));
  close(fd);

  if (nwritten < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  snprintf(path, sizeof(path), "/proc/%d/fpregs", ptid.pid);
  fd = open(path, O_WRONLY);
  if (fd >= 0) {
    double fprs[32];
    for (size_t n = 0; n < 32; n++) {
      fprs[n] = state.ppc.fp.regs[n];
    }

    write(fd, &fprs, sizeof(fprs));
    close(fd);
  }

  return kSuccess;
}

} // namespace SystemV
} // namespace Host
} // namespace ds2
