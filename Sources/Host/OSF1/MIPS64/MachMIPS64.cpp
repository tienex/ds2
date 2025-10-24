//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 MIPS64 Mach CPU state
//

#include "DebugServer2/Host/Mach/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <mach.h>
#include <mach/mips/thread_status.h>
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

  // OSF/1 Mach uses mips_thread_state for MIPS64
  struct mips_thread_state threadState;
  mach_msg_type_number_t stateCount = MIPS_THREAD_STATE_COUNT;

  kern_return_t kret = thread_get_state(
      thread, MIPS_THREAD_STATE, (thread_state_t)&threadState, &stateCount);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_get_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  // Copy MIPS64 register state (64-bit registers)
  // General purpose registers
  memcpy(&state.mips64.gp.regs, &threadState.r, sizeof(state.mips64.gp.regs));

  // Special registers
  state.mips64.special.lo = threadState.mdlo;
  state.mips64.special.hi = threadState.mdhi;
  state.mips64.special.pc = threadState.pc;

  // COP0 registers
  state.mips64.cop0.status = threadState.sr;
  state.mips64.cop0.badvaddr = threadState.badvaddr;
  state.mips64.cop0.cause = threadState.cause;

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

  struct mips_thread_state threadState;

  // General purpose registers
  memcpy(&threadState.r, &state.mips64.gp.regs, sizeof(threadState.r));

  // Special registers
  threadState.mdlo = state.mips64.special.lo;
  threadState.mdhi = state.mips64.special.hi;
  threadState.pc = state.mips64.special.pc;

  // COP0 registers
  threadState.sr = state.mips64.cop0.status;
  threadState.badvaddr = state.mips64.cop0.badvaddr;
  threadState.cause = state.mips64.cop0.cause;

  kern_return_t kret = thread_set_state(
      thread, MIPS_THREAD_STATE, (thread_state_t)&threadState,
      MIPS_THREAD_STATE_COUNT);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "thread_set_state failed: %d", kret);
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

} // namespace Mach
} // namespace Host
} // namespace ds2
