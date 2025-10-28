//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows NT PowerPC Debugging Support (NT 3.51 - NT 4.0)
//

#include "DebugServer2/Host/Windows/Debug.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

namespace ds2 {
namespace Host {
namespace Windows {

ErrorCode Debug::readCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState &state) {
  HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                               FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Windows NT PowerPC CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Copy general-purpose registers (Gpr0-Gpr31)
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = ctx.Gpr0 + n * sizeof(DWORD);
  }

  // Copy special registers
  state.ppc.pc = ctx.Iar;     // Instruction Address Register
  state.ppc.msr = ctx.Msr;    // Machine State Register
  state.ppc.lr = ctx.Lr;      // Link Register
  state.ppc.ctr = ctx.Ctr;    // Count Register
  state.ppc.cr = ctx.Cr;      // Condition Register
  state.ppc.xer = ctx.Xer;    // Fixed-Point Exception Register

  // Copy floating-point registers (Fpr0-Fpr31)
  for (size_t n = 0; n < 32; n++) {
    state.ppc.fp.regs[n] = ctx.Fpr0 + n * sizeof(double);
  }
  state.ppc.fpscr = ctx.Fpscr;

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                               FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Prepare Windows NT PowerPC CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    *(&ctx.Gpr0 + n) = state.ppc.gp.regs[n];
  }

  // Update special registers
  ctx.Iar = state.ppc.pc;
  ctx.Msr = state.ppc.msr;
  ctx.Lr = state.ppc.lr;
  ctx.Ctr = state.ppc.ctr;
  ctx.Cr = state.ppc.cr;
  ctx.Xer = state.ppc.xer;

  // Update floating-point registers
  for (size_t n = 0; n < 32; n++) {
    *(&ctx.Fpr0 + n) = state.ppc.fp.regs[n];
  }
  ctx.Fpscr = state.ppc.fpscr;

  if (!SetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);
  return kSuccess;
}

} // namespace Windows
} // namespace Host
} // namespace ds2
