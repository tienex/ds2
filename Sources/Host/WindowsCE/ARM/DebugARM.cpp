//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows CE ARM Debugging Support
//

#include "DebugServer2/Host/WindowsCE/Debug.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

namespace ds2 {
namespace Host {
namespace WindowsCE {

ErrorCode Debug::readCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState &state) {
  HANDLE hThread = OpenThread(THREAD_GET_CONTEXT, FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Windows CE ARM CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Copy general-purpose registers (R0-R12)
  state.arm.gp.r0 = ctx.R0;
  state.arm.gp.r1 = ctx.R1;
  state.arm.gp.r2 = ctx.R2;
  state.arm.gp.r3 = ctx.R3;
  state.arm.gp.r4 = ctx.R4;
  state.arm.gp.r5 = ctx.R5;
  state.arm.gp.r6 = ctx.R6;
  state.arm.gp.r7 = ctx.R7;
  state.arm.gp.r8 = ctx.R8;
  state.arm.gp.r9 = ctx.R9;
  state.arm.gp.r10 = ctx.R10;
  state.arm.gp.r11 = ctx.R11;
  state.arm.gp.r12 = ctx.R12;

  // Copy special registers
  state.arm.gp.sp = ctx.Sp;
  state.arm.gp.lr = ctx.Lr;
  state.arm.gp.pc = ctx.Pc;
  state.arm.gp.cpsr = ctx.Psr;

  // Copy VFP registers if available
  // Windows CE may have VFP support depending on device
  if (ctx.ContextFlags & CONTEXT_FLOATING_POINT) {
    // Windows CE stores VFP in extended area
    // D0-D15 (VFPv2) or D0-D31 (VFPv3)
    for (size_t n = 0; n < 32 && n < 16; n++) {
      // Windows CE typically supports VFPv2 (D0-D15)
      if (n < 16) {
        state.arm.vfp.d[n] = ((uint64_t *)ctx.ExtendedRegisters)[n];
      }
    }
    state.arm.vfp.fpscr = ctx.Fpscr;
  }

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Prepare Windows CE ARM CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update general-purpose registers
  ctx.R0 = state.arm.gp.r0;
  ctx.R1 = state.arm.gp.r1;
  ctx.R2 = state.arm.gp.r2;
  ctx.R3 = state.arm.gp.r3;
  ctx.R4 = state.arm.gp.r4;
  ctx.R5 = state.arm.gp.r5;
  ctx.R6 = state.arm.gp.r6;
  ctx.R7 = state.arm.gp.r7;
  ctx.R8 = state.arm.gp.r8;
  ctx.R9 = state.arm.gp.r9;
  ctx.R10 = state.arm.gp.r10;
  ctx.R11 = state.arm.gp.r11;
  ctx.R12 = state.arm.gp.r12;

  // Update special registers
  ctx.Sp = state.arm.gp.sp;
  ctx.Lr = state.arm.gp.lr;
  ctx.Pc = state.arm.gp.pc;
  ctx.Psr = state.arm.gp.cpsr;

  // Update VFP registers if available
  if (ctx.ContextFlags & CONTEXT_FLOATING_POINT) {
    for (size_t n = 0; n < 32 && n < 16; n++) {
      if (n < 16) {
        ((uint64_t *)ctx.ExtendedRegisters)[n] = state.arm.vfp.d[n];
      }
    }
    ctx.Fpscr = state.arm.vfp.fpscr;
  }

  if (!SetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);
  return kSuccess;
}

} // namespace WindowsCE
} // namespace Host
} // namespace ds2
