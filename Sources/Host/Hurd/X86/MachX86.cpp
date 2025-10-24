//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// GNU/Hurd X86 (32-bit) Mach CPU state
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

  // GNU Mach uses i386_thread_state for 32-bit x86
  struct i386_thread_state threadState;
  mach_msg_type_number_t stateCount = i386_THREAD_STATE_COUNT;

  kern_return_t kret = thread_get_state(
      thread, i386_THREAD_STATE, (thread_state_t)&threadState, &stateCount);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_get_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  // Copy 32-bit register state
  state.is32 = true;
  state.state32.gp.eax = threadState.eax;
  state.state32.gp.ecx = threadState.ecx;
  state.state32.gp.edx = threadState.edx;
  state.state32.gp.ebx = threadState.ebx;
  state.state32.gp.esi = threadState.esi;
  state.state32.gp.edi = threadState.edi;
  state.state32.gp.ebp = threadState.ebp;
  state.state32.gp.esp = threadState.uesp;
  state.state32.gp.eip = threadState.eip;
  state.state32.gp.cs = threadState.cs & 0xffff;
  state.state32.gp.ss = threadState.ss & 0xffff;
  state.state32.gp.ds = threadState.ds & 0xffff;
  state.state32.gp.es = threadState.es & 0xffff;
  state.state32.gp.fs = threadState.fs & 0xffff;
  state.state32.gp.gs = threadState.gs & 0xffff;
  state.state32.gp.eflags = threadState.efl;

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

  threadState.eax = state.state32.gp.eax;
  threadState.ecx = state.state32.gp.ecx;
  threadState.edx = state.state32.gp.edx;
  threadState.ebx = state.state32.gp.ebx;
  threadState.esi = state.state32.gp.esi;
  threadState.edi = state.state32.gp.edi;
  threadState.ebp = state.state32.gp.ebp;
  threadState.uesp = state.state32.gp.esp;
  threadState.eip = state.state32.gp.eip;
  threadState.cs = state.state32.gp.cs & 0xffff;
  threadState.ss = state.state32.gp.ss & 0xffff;
  threadState.ds = state.state32.gp.ds & 0xffff;
  threadState.es = state.state32.gp.es & 0xffff;
  threadState.fs = state.state32.gp.fs & 0xffff;
  threadState.gs = state.state32.gp.gs & 0xffff;
  threadState.efl = state.state32.gp.eflags;

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
