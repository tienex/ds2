//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Host/BeOS/Platform.h"
#include "DebugServer2/Host/Platform.h"

#include <OS.h>
#include <debugger.h>

namespace ds2 {
namespace Host {
namespace BeOS {

using namespace Architecture::PowerPC;

ErrorCode ReadCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       Architecture::CPUState &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  debug_cpu_state cpuState;
  if (Platform::GetThreadState(ptid.tid, cpuState) != kSuccess)
    return Platform::TranslateError();

  // Map BeOS PowerPC debug_cpu_state to our CPUState
  // BeOS PowerPC structure contains GPRs, special registers, etc.
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = cpuState.r[i];
  }

  state.special.pc = cpuState.pc;
  state.special.lr = cpuState.lr;
  state.special.ctr = cpuState.ctr;
  state.special.cr = cpuState.cr;
  state.special.xer = cpuState.xer;
  state.special.msr = cpuState.msr;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    state.fpu.regs[i] = cpuState.f[i];
  }
  state.fpu_ctrl.fpscr = cpuState.fpscr;

  return kSuccess;
}

ErrorCode WriteCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                        Architecture::CPUState const &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  debug_cpu_state cpuState;

  // Map our CPUState to BeOS PowerPC debug_cpu_state
  for (int i = 0; i < 32; i++) {
    cpuState.r[i] = state.gp.regs[i];
  }

  cpuState.pc = state.special.pc;
  cpuState.lr = state.special.lr;
  cpuState.ctr = state.special.ctr;
  cpuState.cr = state.special.cr;
  cpuState.xer = state.special.xer;
  cpuState.msr = state.special.msr;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    cpuState.f[i] = state.fpu.regs[i];
  }
  cpuState.fpscr = state.fpu_ctrl.fpscr;

  if (Platform::SetThreadState(ptid.tid, cpuState) != kSuccess)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace BeOS
} // namespace Host
} // namespace ds2
