//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// GNU/Hurd X86_64 Mach CPU state - adapted from Darwin
//

#include "DebugServer2/Host/Mach/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <mach.h>
#include <mach/i386/thread_status.h>
#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Mach {

ErrorCode MachInterface::readCPUState(ProcessThreadId const &ptid,
                             ProcessInfo const &pinfo,
                             Architecture::CPUState &state) {
  if (!ptid.valid()) {
    return kErrorInvalidArgument;
  }

  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  // GNU Mach uses i386_thread_state for x86_64
  struct i386_thread_state threadState;
  mach_msg_type_number_t stateCount = i386_THREAD_STATE_COUNT;

  kern_return_t kret = thread_get_state(
      thread, i386_THREAD_STATE, (thread_state_t)&threadState, &stateCount);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_get_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  // GNU Mach on x86_64: Copy register state
  state.is32 = false;
  state.state64.gp.rax = threadState.rax;
  state.state64.gp.rcx = threadState.rcx;
  state.state64.gp.rdx = threadState.rdx;
  state.state64.gp.rbx = threadState.rbx;
  state.state64.gp.rsi = threadState.rsi;
  state.state64.gp.rdi = threadState.rdi;
  state.state64.gp.rbp = threadState.rbp;
  state.state64.gp.rsp = threadState.ursp;
  state.state64.gp.r8 = threadState.r8;
  state.state64.gp.r9 = threadState.r9;
  state.state64.gp.r10 = threadState.r10;
  state.state64.gp.r11 = threadState.r11;
  state.state64.gp.r12 = threadState.r12;
  state.state64.gp.r13 = threadState.r13;
  state.state64.gp.r14 = threadState.r14;
  state.state64.gp.r15 = threadState.r15;
  state.state64.gp.rip = threadState.rip;
  state.state64.gp.cs = threadState.cs & 0xffff;
  state.state64.gp.ss = threadState.ss & 0xffff;
  state.state64.gp.ds = threadState.ds & 0xffff;
  state.state64.gp.es = threadState.es & 0xffff;
  state.state64.gp.fs = threadState.fs & 0xffff;
  state.state64.gp.gs = threadState.gs & 0xffff;
  state.state64.gp.eflags = threadState.rfl;

  return kSuccess;
}

ErrorCode MachInterface::writeCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState const &state) {
  if (!ptid.valid()) {
    return kErrorInvalidArgument;
  }

  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  struct i386_thread_state threadState;

  threadState.rax = state.state64.gp.rax;
  threadState.rcx = state.state64.gp.rcx;
  threadState.rdx = state.state64.gp.rdx;
  threadState.rbx = state.state64.gp.rbx;
  threadState.rsi = state.state64.gp.rsi;
  threadState.rdi = state.state64.gp.rdi;
  threadState.rbp = state.state64.gp.rbp;
  threadState.ursp = state.state64.gp.rsp;
  threadState.r8 = state.state64.gp.r8;
  threadState.r9 = state.state64.gp.r9;
  threadState.r10 = state.state64.gp.r10;
  threadState.r11 = state.state64.gp.r11;
  threadState.r12 = state.state64.gp.r12;
  threadState.r13 = state.state64.gp.r13;
  threadState.r14 = state.state64.gp.r14;
  threadState.r15 = state.state64.gp.r15;
  threadState.rip = state.state64.gp.rip;
  threadState.cs = state.state64.gp.cs & 0xffff;
  threadState.ss = state.state64.gp.ss & 0xffff;
  threadState.ds = state.state64.gp.ds & 0xffff;
  threadState.es = state.state64.gp.es & 0xffff;
  threadState.fs = state.state64.gp.fs & 0xffff;
  threadState.gs = state.state64.gp.gs & 0xffff;
  threadState.rfl = state.state64.gp.eflags;

  kern_return_t kret = thread_set_state(
      thread, i386_THREAD_STATE, (thread_state_t)&threadState,
      i386_THREAD_STATE_COUNT);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_set_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

} // namespace Mach
} // namespace Host
} // namespace ds2
