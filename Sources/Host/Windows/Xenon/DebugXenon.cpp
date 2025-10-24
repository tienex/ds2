//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Xbox 360 Xenon (PowerPC64 with VMX128) Debugging Support
//

#include "DebugServer2/Host/Windows/Debug.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Architecture/PowerPC/VMX128.h"
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

  // Xbox 360 CONTEXT structure with VMX128 support
  // This extends the standard PowerPC CONTEXT with 128 vector registers
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Copy general-purpose registers (64-bit on Xenon)
  for (size_t n = 0; n < 32; n++) {
    state.xenon.gp.regs[n] = ctx.Gpr[n];
  }

  // Copy special registers
  state.xenon.pc = ctx.Iar;      // Instruction Address Register
  state.xenon.msr = ctx.Msr;     // Machine State Register
  state.xenon.lr = ctx.Lr;       // Link Register
  state.xenon.ctr = ctx.Ctr;     // Count Register
  state.xenon.cr = ctx.Cr;       // Condition Register
  state.xenon.xer = ctx.Xer;     // Fixed-Point Exception Register
  state.xenon.dar = ctx.Dar;     // Data Address Register
  state.xenon.dsisr = ctx.Dsisr; // DSISR

  // Copy floating-point registers
  for (size_t n = 0; n < 32; n++) {
    state.xenon.fp.regs[n] = ctx.Fpr[n];
  }
  state.xenon.fpscr = ctx.Fpscr;

  // Copy VMX128 vector registers (128 registers on Xenon vs 32 on standard PowerPC)
  // The extended registers area contains the VMX128 state
  if (ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS) {
    // Xbox 360 stores VMX128 in extended register area
    // Each vector register is 128 bits (4 x 32-bit words)
    uint32_t *vmxData = (uint32_t *)ctx.ExtendedRegisters;
    for (size_t n = 0; n < 128; n++) {
      for (size_t i = 0; i < 4; i++) {
        state.xenon.vr[n].v[i] = vmxData[n * 4 + i];
      }
    }

    // VSCR is stored after the vector registers
    state.xenon.vscr = vmxData[128 * 4];
  }

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                               FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Prepare Xbox 360 CONTEXT structure with VMX128 support
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update general-purpose registers (64-bit)
  for (size_t n = 0; n < 32; n++) {
    ctx.Gpr[n] = state.xenon.gp.regs[n];
  }

  // Update special registers
  ctx.Iar = state.xenon.pc;
  ctx.Msr = state.xenon.msr;
  ctx.Lr = state.xenon.lr;
  ctx.Ctr = state.xenon.ctr;
  ctx.Cr = state.xenon.cr;
  ctx.Xer = state.xenon.xer;
  ctx.Dar = state.xenon.dar;
  ctx.Dsisr = state.xenon.dsisr;

  // Update floating-point registers
  for (size_t n = 0; n < 32; n++) {
    ctx.Fpr[n] = state.xenon.fp.regs[n];
  }
  ctx.Fpscr = state.xenon.fpscr;

  // Update VMX128 vector registers (128 registers)
  if (ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS) {
    uint32_t *vmxData = (uint32_t *)ctx.ExtendedRegisters;
    for (size_t n = 0; n < 128; n++) {
      for (size_t i = 0; i < 4; i++) {
        vmxData[n * 4 + i] = state.xenon.vr[n].v[i];
      }
    }

    // Update VSCR
    vmxData[128 * 4] = state.xenon.vscr;
  }

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
