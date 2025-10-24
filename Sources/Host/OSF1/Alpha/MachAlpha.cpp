//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 Alpha Mach CPU state
//

#include "DebugServer2/Host/Mach/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <mach.h>
#include <mach/alpha/thread_status.h>
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

  // OSF/1 Mach uses alpha_thread_state for Alpha
  struct alpha_thread_state threadState;
  mach_msg_type_number_t stateCount = ALPHA_THREAD_STATE_COUNT;

  kern_return_t kret = thread_get_state(
      thread, ALPHA_THREAD_STATE, (thread_state_t)&threadState, &stateCount);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_get_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  // Copy Alpha register state
  // Integer registers (r0-r31)
  memcpy(&state.alpha.gp.regs, &threadState.r, sizeof(state.alpha.gp.regs));

  // Floating-point registers (f0-f31)
  memcpy(&state.alpha.fp.raw, &threadState.f, sizeof(state.alpha.fp.raw));

  // Special registers
  state.alpha.pc = threadState.pc;
  state.alpha.fpcr = threadState.fpcr;
  state.alpha.unique = threadState.unique;

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

  struct alpha_thread_state threadState;

  // Integer registers (r0-r31)
  memcpy(&threadState.r, &state.alpha.gp.regs, sizeof(threadState.r));

  // Floating-point registers (f0-f31)
  memcpy(&threadState.f, &state.alpha.fp.raw, sizeof(threadState.f));

  // Special registers
  threadState.pc = state.alpha.pc;
  threadState.fpcr = state.alpha.fpcr;
  threadState.unique = state.alpha.unique;

  kern_return_t kret = thread_set_state(
      thread, ALPHA_THREAD_STATE, (thread_state_t)&threadState,
      ALPHA_THREAD_STATE_COUNT);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_set_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

} // namespace Mach
} // namespace Host
} // namespace ds2
