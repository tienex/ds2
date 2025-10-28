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
#include "DebugServer2/Host/BeOS/Platform.h"
#include "DebugServer2/Host/Platform.h"

#include <OS.h>
#include <debugger.h>

namespace ds2 {
namespace Host {
namespace BeOS {

using namespace Architecture::X86;

ErrorCode ReadCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       Architecture::CPUState &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  debug_cpu_state cpuState;
  if (Platform::GetThreadState(ptid.tid, cpuState) != kSuccess)
    return Platform::TranslateError();

  // Map BeOS x86 debug_cpu_state to our CPUState
  // BeOS x86 structure contains standard x86 registers
  state.gp.eax = cpuState.eax;
  state.gp.ebx = cpuState.ebx;
  state.gp.ecx = cpuState.ecx;
  state.gp.edx = cpuState.edx;
  state.gp.esi = cpuState.esi;
  state.gp.edi = cpuState.edi;
  state.gp.ebp = cpuState.ebp;
  state.gp.esp = cpuState.esp;
  state.gp.eip = cpuState.eip;
  state.gp.eflags = cpuState.eflags;

  state.gp.cs = cpuState.cs;
  state.gp.ss = cpuState.ss;
  state.gp.ds = cpuState.ds;
  state.gp.es = cpuState.es;
  state.gp.fs = cpuState.fs;
  state.gp.gs = cpuState.gs;

  // Note: BeOS may not expose all FPU state through debug API
  // FPU state would need to be read separately if available

  return kSuccess;
}

ErrorCode WriteCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                        Architecture::CPUState const &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  debug_cpu_state cpuState;

  // Map our CPUState to BeOS x86 debug_cpu_state
  cpuState.eax = state.gp.eax;
  cpuState.ebx = state.gp.ebx;
  cpuState.ecx = state.gp.ecx;
  cpuState.edx = state.gp.edx;
  cpuState.esi = state.gp.esi;
  cpuState.edi = state.gp.edi;
  cpuState.ebp = state.gp.ebp;
  cpuState.esp = state.gp.esp;
  cpuState.eip = state.gp.eip;
  cpuState.eflags = state.gp.eflags;

  cpuState.cs = state.gp.cs;
  cpuState.ss = state.gp.ss;
  cpuState.ds = state.gp.ds;
  cpuState.es = state.gp.es;
  cpuState.fs = state.gp.fs;
  cpuState.gs = state.gp.gs;

  if (Platform::SetThreadState(ptid.tid, cpuState) != kSuccess)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace BeOS
} // namespace Host
} // namespace ds2
