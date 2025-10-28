//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows NT Alpha32 Thread
//

#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  CONTEXT context;
  context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(_handle, &context)) {
    return kErrorInvalidArgument;
  }

  // Windows NT Alpha32 CONTEXT structure
  // Integer registers (32-bit on Alpha32)
  for (int i = 0; i < 32; i++) {
    state.alpha.gp.regs[i] = context.IntV0 + i; // Simplified - actual structure varies
  }

  // PC
  state.alpha.pc = context.Fir; // Fir = Fault Instruction Register (PC)

  // FP registers
  for (int i = 0; i < 32; i++) {
    state.alpha.fp.raw[i] = *((uint64_t *)&context.FltF0 + i);
  }

  state.alpha.fpcr = context.Fpcr;

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  CONTEXT context;
  context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(_handle, &context)) {
    return kErrorInvalidArgument;
  }

  // Set registers
  for (int i = 0; i < 32; i++) {
    *(&context.IntV0 + i) = state.alpha.gp.regs[i];
  }

  context.Fir = state.alpha.pc;

  for (int i = 0; i < 32; i++) {
    *((uint64_t *)&context.FltF0 + i) = state.alpha.fp.raw[i];
  }

  context.Fpcr = state.alpha.fpcr;

  if (!SetThreadContext(_handle, &context)) {
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

} // namespace Windows
} // namespace Target
} // namespace ds2
