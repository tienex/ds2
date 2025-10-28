//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows CE PowerPC Debugging Support
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

  // Windows CE PowerPC CONTEXT structure (simplified compared to NT)
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Copy general-purpose registers
  // Windows CE uses Gpr array
  for (size_t n = 0; n < 32; n++) {
    state.ppc.gp.regs[n] = ctx.Gpr[n];
  }

  // Copy special registers
  state.ppc.pc = ctx.Iar;
  state.ppc.msr = ctx.Msr;
  state.ppc.lr = ctx.Lr;
  state.ppc.ctr = ctx.Ctr;
  state.ppc.cr = ctx.Cr;
  state.ppc.xer = ctx.Xer;

  // Copy floating-point registers
  for (size_t n = 0; n < 32; n++) {
    state.ppc.fp.regs[n] = ctx.Fpr[n];
  }
  state.ppc.fpscr = ctx.Fpscr;

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Prepare Windows CE PowerPC CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    ctx.Gpr[n] = state.ppc.gp.regs[n];
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
    ctx.Fpr[n] = state.ppc.fp.regs[n];
  }
  ctx.Fpscr = state.ppc.fpscr;

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
