//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Windows {

//
// Software single-step for MIPS
// Note: MIPS doesn't have hardware single-step, must use breakpoints
//
ErrorCode Thread::step(int signal, Address const &address) {
  if (_state == kInvalid || _state == kRunning) {
    return kErrorInvalidArgument;
  } else if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  DS2LOG(Debug, "stepping tid %d", tid());

  //
  // TODO: Implement software single-step for MIPS
  // This requires:
  // 1. Decode the current instruction
  // 2. Calculate next PC (accounting for branches/jumps)
  // 3. Set breakpoint at next PC
  // 4. Resume execution
  //
  // For now, return unsupported
  return kErrorUnsupported;
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  CONTEXT context;

  memset(&context, 0, sizeof(context));

#if defined(_MIPS_)
  // MIPS-specific context flags
  context.ContextFlags = CONTEXT_INTEGER |        // GP registers
                         CONTEXT_CONTROL |        // PC, status
                         CONTEXT_FLOATING_POINT;  // FP registers

  BOOL result = GetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }

  //
  // Copy general purpose registers
  // Windows CONTEXT structure for MIPS has:
  // IntZero, IntAt, IntV0, IntV1, IntA0-IntA3, IntT0-IntT9,
  // IntS0-IntS8, IntK0-IntK1, IntGp, IntSp, IntS8, IntRa
  //
  state.gp.zero = context.IntZero;
  state.gp.at = context.IntAt;
  state.gp.v0 = context.IntV0;
  state.gp.v1 = context.IntV1;
  state.gp.a0 = context.IntA0;
  state.gp.a1 = context.IntA1;
  state.gp.a2 = context.IntA2;
  state.gp.a3 = context.IntA3;
  state.gp.t0 = context.IntT0;
  state.gp.t1 = context.IntT1;
  state.gp.t2 = context.IntT2;
  state.gp.t3 = context.IntT3;
  state.gp.t4 = context.IntT4;
  state.gp.t5 = context.IntT5;
  state.gp.t6 = context.IntT6;
  state.gp.t7 = context.IntT7;
  state.gp.s0 = context.IntS0;
  state.gp.s1 = context.IntS1;
  state.gp.s2 = context.IntS2;
  state.gp.s3 = context.IntS3;
  state.gp.s4 = context.IntS4;
  state.gp.s5 = context.IntS5;
  state.gp.s6 = context.IntS6;
  state.gp.s7 = context.IntS7;
  state.gp.t8 = context.IntT8;
  state.gp.t9 = context.IntT9;
  state.gp.k0 = context.IntK0;
  state.gp.k1 = context.IntK1;
  state.gp.gp = context.IntGp;
  state.gp.sp = context.IntSp;
  state.gp.s8 = context.IntS8;  // Frame pointer
  state.gp.ra = context.IntRa;

  // Special registers
  state.special.lo = context.IntLo;
  state.special.hi = context.IntHi;
  state.special.pc = context.Fir;  // PC is in Fir (Fetch Instruction Register)

  // Status registers
  state.cop0.status = context.Psr;  // Processor Status Register

  // Floating point registers
  for (size_t i = 0; i < 32; ++i) {
    state.fpu.sng[i] = context.FltF[i];
  }
  state.fpu.fcsr = context.Fsr;  // FP Status Register
#else
  // Not compiling for MIPS, return error
  return kErrorUnsupported;
#endif

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  CONTEXT context;

  memset(&context, 0, sizeof(context));

#if defined(_MIPS_)
  context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL |
                         CONTEXT_FLOATING_POINT;

  // Read current context first
  BOOL result = GetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }

  //
  // Update general purpose registers
  //
  context.IntZero = state.gp.zero;
  context.IntAt = state.gp.at;
  context.IntV0 = state.gp.v0;
  context.IntV1 = state.gp.v1;
  context.IntA0 = state.gp.a0;
  context.IntA1 = state.gp.a1;
  context.IntA2 = state.gp.a2;
  context.IntA3 = state.gp.a3;
  context.IntT0 = state.gp.t0;
  context.IntT1 = state.gp.t1;
  context.IntT2 = state.gp.t2;
  context.IntT3 = state.gp.t3;
  context.IntT4 = state.gp.t4;
  context.IntT5 = state.gp.t5;
  context.IntT6 = state.gp.t6;
  context.IntT7 = state.gp.t7;
  context.IntS0 = state.gp.s0;
  context.IntS1 = state.gp.s1;
  context.IntS2 = state.gp.s2;
  context.IntS3 = state.gp.s3;
  context.IntS4 = state.gp.s4;
  context.IntS5 = state.gp.s5;
  context.IntS6 = state.gp.s6;
  context.IntS7 = state.gp.s7;
  context.IntT8 = state.gp.t8;
  context.IntT9 = state.gp.t9;
  context.IntK0 = state.gp.k0;
  context.IntK1 = state.gp.k1;
  context.IntGp = state.gp.gp;
  context.IntSp = state.gp.sp;
  context.IntS8 = state.gp.s8;
  context.IntRa = state.gp.ra;

  // Special registers
  context.IntLo = state.special.lo;
  context.IntHi = state.special.hi;
  context.Fir = state.special.pc;

  // Status
  context.Psr = state.cop0.status;

  // Floating point registers
  for (size_t i = 0; i < 32; ++i) {
    context.FltF[i] = state.fpu.sng[i];
  }
  context.Fsr = state.fpu.fcsr;

  // Write back
  result = SetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }
#else
  return kErrorUnsupported;
#endif

  return kSuccess;
}

} // namespace Windows
} // namespace Target
} // namespace ds2
