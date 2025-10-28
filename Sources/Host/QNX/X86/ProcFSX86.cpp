//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/QNX/ProcFS.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/debug.h>
#include <sys/procfs.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace QNX {

using namespace Architecture::X86;

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Open /proc/<pid>/as for process memory and control
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/as", pid);

  int fd = ::open(path, O_RDWR);
  if (fd < 0)
    return Platform::TranslateError();

  // QNX procfs uses debug_thread_t for thread operations
  debug_thread_t thread_info;
  memset(&thread_info, 0, sizeof(thread_info));
  thread_info.tid = ptid.tid;

  // Get thread register state using devctl
  int status = ::devctl(fd, DCMD_PROC_TIDSTATUS, &thread_info,
                       sizeof(thread_info), NULL);
  if (status != EOK) {
    ::close(fd);
    errno = status;
    return Platform::TranslateError();
  }

  // Get x86 CPU context
  X86_CPU_REGISTERS *regs = &thread_info.cpu;

  // Map QNX x86 registers to our CPUState
  state.gp.eax = regs->eax;
  state.gp.ebx = regs->ebx;
  state.gp.ecx = regs->ecx;
  state.gp.edx = regs->edx;
  state.gp.esi = regs->esi;
  state.gp.edi = regs->edi;
  state.gp.ebp = regs->ebp;
  state.gp.esp = regs->esp;
  state.gp.eip = regs->eip;
  state.gp.eflags = regs->efl;

  state.gp.cs = regs->cs;
  state.gp.ss = regs->ss;
  state.gp.ds = regs->ds;
  state.gp.es = regs->es;
  state.gp.fs = regs->fs;
  state.gp.gs = regs->gs;

  // Get FPU state if available
  X86_FPU_REGISTERS *fpu = &thread_info.fpu;
  for (int i = 0; i < 8; i++) {
    // QNX stores FPU registers in 80-bit extended format
    memcpy(&state.x87.regs[i], &fpu->st[i], 10);
  }
  state.x87.fctrl = fpu->cw;
  state.x87.fstat = fpu->sw;
  state.x87.ftag = fpu->tag;

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

  // Open /proc/<pid>/as
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/as", pid);

  int fd = ::open(path, O_RDWR);
  if (fd < 0)
    return Platform::TranslateError();

  // Get current thread state first
  debug_thread_t thread_info;
  memset(&thread_info, 0, sizeof(thread_info));
  thread_info.tid = ptid.tid;

  int status = ::devctl(fd, DCMD_PROC_TIDSTATUS, &thread_info,
                       sizeof(thread_info), NULL);
  if (status != EOK) {
    ::close(fd);
    errno = status;
    return Platform::TranslateError();
  }

  // Update x86 registers
  X86_CPU_REGISTERS *regs = &thread_info.cpu;

  regs->eax = state.gp.eax;
  regs->ebx = state.gp.ebx;
  regs->ecx = state.gp.ecx;
  regs->edx = state.gp.edx;
  regs->esi = state.gp.esi;
  regs->edi = state.gp.edi;
  regs->ebp = state.gp.ebp;
  regs->esp = state.gp.esp;
  regs->eip = state.gp.eip;
  regs->efl = state.gp.eflags;

  regs->cs = state.gp.cs;
  regs->ss = state.gp.ss;
  regs->ds = state.gp.ds;
  regs->es = state.gp.es;
  regs->fs = state.gp.fs;
  regs->gs = state.gp.gs;

  // Update FPU state
  X86_FPU_REGISTERS *fpu = &thread_info.fpu;
  for (int i = 0; i < 8; i++) {
    memcpy(&fpu->st[i], &state.x87.regs[i], 10);
  }
  fpu->cw = state.x87.fctrl;
  fpu->sw = state.x87.fstat;
  fpu->tag = state.x87.ftag;

  // Write back the thread state
  status = ::devctl(fd, DCMD_PROC_SET_REG, &thread_info, sizeof(thread_info),
                   NULL);
  if (status != EOK) {
    ::close(fd);
    errno = status;
    return Platform::TranslateError();
  }

  ::close(fd);
  return kSuccess;
}

} // namespace QNX
} // namespace Host
} // namespace ds2
