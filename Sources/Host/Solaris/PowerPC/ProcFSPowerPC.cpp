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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace SystemV {

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  // Solaris uses /proc/<pid>/lwp/<lwpid>/lwpstatus for thread state
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpstatus",
           ptid.pid, ptid.tid);

  int fd = open(path, O_RDONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Solaris lwpstatus structure contains register information
  lwpstatus_t lwpstat;
  ssize_t nread = read(fd, &lwpstat, sizeof(lwpstat));
  close(fd);

  if (nread < 0)
    return Platform::TranslateError();

  // Extract PowerPC registers from lwpstatus
  // Solaris uses prgregset_t for general registers
  prgregset_t *regs = &lwpstat.pr_reg;

  // Copy general-purpose registers (Solaris PowerPC layout)
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = (*regs)[n];
  }

  // Special registers follow GPRs in Solaris layout
  state.ppc.pc = (*regs)[32];   // NIP
  state.ppc.msr = (*regs)[33];  // MSR
  state.ppc.cr = (*regs)[34];   // CR
  state.ppc.lr = (*regs)[35];   // LR
  state.ppc.ctr = (*regs)[36];  // CTR
  state.ppc.xer = (*regs)[37];  // XER

  // Read floating-point registers
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl",
           ptid.pid, ptid.tid);
  fd = open(path, O_RDONLY);
  if (fd >= 0) {
    // Solaris provides floating-point state via separate mechanism
    prfpregset_t fprs;
    if (read(fd, &fprs, sizeof(fprs)) > 0) {
      for (size_t n = 0; n < 32; n++) {
        state.ppc.fp.regs[n] = fprs.pr_fr.pr_regs[n];
      }
      state.ppc.fpscr = fprs.pr_fsr;
    }
    close(fd);
  }

  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  // Solaris uses /proc/<pid>/lwp/<lwpid>/lwpctl for control operations
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl",
           ptid.pid, ptid.tid);

  int ctlfd = open(path, O_WRONLY);
  if (ctlfd < 0)
    return Platform::TranslateError();

  // Prepare register set
  prgregset_t regs;

  for (size_t n = 0; n < 32; n++) {
    regs[n] = state.ppc.gp.regs[n];
  }

  regs[32] = state.ppc.pc;
  regs[33] = state.ppc.msr;
  regs[34] = state.ppc.cr;
  regs[35] = state.ppc.lr;
  regs[36] = state.ppc.ctr;
  regs[37] = state.ppc.xer;

  // Use PCSREG command to set registers
  struct {
    long cmd;
    prgregset_t regs;
  } ctl;

  ctl.cmd = PCSREG;
  memcpy(&ctl.regs, &regs, sizeof(regs));

  ssize_t nwritten = write(ctlfd, &ctl, sizeof(ctl));
  close(ctlfd);

  if (nwritten < 0)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace SystemV
} // namespace Host
} // namespace ds2
