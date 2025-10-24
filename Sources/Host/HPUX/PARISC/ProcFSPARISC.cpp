//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PARISC/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/procfs.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace HPUX {

using namespace Architecture::PARISC;

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Open /proc/<pid>/lwp/<tid>/lwpctl for thread control
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", pid, ptid.tid);

  int fd = ::open(path, O_RDWR);
  if (fd < 0)
    return Platform::TranslateError();

  // Read general purpose registers using prgregset_t
  prgregset_t gregs;
  if (::pread(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  // Map HP-UX prgregset_t to our CPUState
  // prgregset_t layout for PA-RISC:
  // gr[0-31], sr[0-7], iaoq[0-1], iasq[0-1], sar, pcoq[0-1], pcsq[0-1], etc.

  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = gregs[i];
  }

  // Space registers
  for (int i = 0; i < 8; i++) {
    state.sr.regs[i] = gregs[32 + i];
  }

  // Special registers
  state.special.iaoq_head = gregs[40]; // IAOQ head
  state.special.iaoq_tail = gregs[41]; // IAOQ tail
  state.special.iasq_head = gregs[42]; // IASQ head
  state.special.iasq_tail = gregs[43]; // IASQ tail

  state.special.sar = gregs[44];       // SAR

  state.special.pcoq_head = gregs[45]; // PCOQ head
  state.special.pcoq_tail = gregs[46]; // PCOQ tail
  state.special.pcsq_head = gregs[47]; // PCSQ head
  state.special.pcsq_tail = gregs[48]; // PCSQ tail

  state.special.psw = gregs[49];       // PSW

  // Read floating point registers
  char fppath[128];
  snprintf(fppath, sizeof(fppath), "/proc/%d/lwp/%d/lwpctl", pid, ptid.tid);

  prfpregset_t fpregs;
  if (::pread(fd, &fpregs, sizeof(fpregs), sizeof(gregs)) ==
      sizeof(fpregs)) {
    // Copy FP registers
    for (int i = 0; i < 32; i++) {
      state.fpu.l[i] = fpregs.fpr[i];
    }
    state.fpu_status.fpsr = fpregs.fpsr;
  }

  ::close(fd);
  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Open /proc/<pid>/lwp/<tid>/lwpctl
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", pid, ptid.tid);

  int fd = ::open(path, O_RDWR);
  if (fd < 0)
    return Platform::TranslateError();

  // Read current state first
  prgregset_t gregs;
  if (::pread(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  // Update registers from our CPUState
  for (int i = 0; i < 32; i++) {
    gregs[i] = state.gp.regs[i];
  }

  // Space registers
  for (int i = 0; i < 8; i++) {
    gregs[32 + i] = state.sr.regs[i];
  }

  // Special registers
  gregs[40] = state.special.iaoq_head;
  gregs[41] = state.special.iaoq_tail;
  gregs[42] = state.special.iasq_head;
  gregs[43] = state.special.iasq_tail;
  gregs[44] = state.special.sar;
  gregs[45] = state.special.pcoq_head;
  gregs[46] = state.special.pcoq_tail;
  gregs[47] = state.special.pcsq_head;
  gregs[48] = state.special.pcsq_tail;
  gregs[49] = state.special.psw;

  // Write back general purpose registers
  if (::pwrite(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  // Write floating point registers
  prfpregset_t fpregs;
  for (int i = 0; i < 32; i++) {
    fpregs.fpr[i] = state.fpu.l[i];
  }
  fpregs.fpsr = state.fpu_status.fpsr;

  if (::pwrite(fd, &fpregs, sizeof(fpregs), sizeof(gregs)) !=
      sizeof(fpregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);
  return kSuccess;
}

} // namespace HPUX
} // namespace Host
} // namespace ds2
