//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows CE x86 Debugging Support
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

  // Windows CE x86 CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Copy general-purpose registers
  state.x86.gp.eax = ctx.Eax;
  state.x86.gp.ecx = ctx.Ecx;
  state.x86.gp.edx = ctx.Edx;
  state.x86.gp.ebx = ctx.Ebx;
  state.x86.gp.esp = ctx.Esp;
  state.x86.gp.ebp = ctx.Ebp;
  state.x86.gp.esi = ctx.Esi;
  state.x86.gp.edi = ctx.Edi;

  // Copy segment registers
  state.x86.segments.cs = ctx.SegCs;
  state.x86.segments.ss = ctx.SegSs;
  state.x86.segments.ds = ctx.SegDs;
  state.x86.segments.es = ctx.SegEs;
  state.x86.segments.fs = ctx.SegFs;
  state.x86.segments.gs = ctx.SegGs;

  // Copy control registers
  state.x86.gp.eip = ctx.Eip;
  state.x86.gp.eflags = ctx.EFlags;

  // Copy x87 FPU registers
  if (ctx.ContextFlags & CONTEXT_FLOATING_POINT) {
    state.x86.fpu.fctrl = ctx.FloatSave.ControlWord;
    state.x86.fpu.fstat = ctx.FloatSave.StatusWord;
    state.x86.fpu.ftag = ctx.FloatSave.TagWord;
    state.x86.fpu.fop = ctx.FloatSave.ErrorOpcode;
    state.x86.fpu.fiseg = ctx.FloatSave.ErrorSelector;
    state.x86.fpu.fioff = ctx.FloatSave.ErrorOffset;
    state.x86.fpu.foseg = ctx.FloatSave.DataSelector;
    state.x86.fpu.fooff = ctx.FloatSave.DataOffset;

    // Copy ST0-ST7 registers (80-bit each)
    for (size_t n = 0; n < 8; n++) {
      memcpy(&state.x86.fpu.st[n], &ctx.FloatSave.RegisterArea[n * 10], 10);
    }
  }

  // Copy SSE registers if available (Windows CE may have limited SSE support)
  if (ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS) {
    // Extended registers contain SSE state (XMM0-XMM7 on x86)
    uint8_t *extRegs = (uint8_t *)ctx.ExtendedRegisters;

    // MXCSR is at offset 24
    state.x86.sse.mxcsr = *(uint32_t *)(extRegs + 24);

    // XMM registers start at offset 160
    for (size_t n = 0; n < 8; n++) {
      memcpy(&state.x86.sse.xmm[n], extRegs + 160 + n * 16, 16);
    }
  }

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState const &state) {
  HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Prepare Windows CE x86 CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update general-purpose registers
  ctx.Eax = state.x86.gp.eax;
  ctx.Ecx = state.x86.gp.ecx;
  ctx.Edx = state.x86.gp.edx;
  ctx.Ebx = state.x86.gp.ebx;
  ctx.Esp = state.x86.gp.esp;
  ctx.Ebp = state.x86.gp.ebp;
  ctx.Esi = state.x86.gp.esi;
  ctx.Edi = state.x86.gp.edi;

  // Update segment registers
  ctx.SegCs = state.x86.segments.cs;
  ctx.SegSs = state.x86.segments.ss;
  ctx.SegDs = state.x86.segments.ds;
  ctx.SegEs = state.x86.segments.es;
  ctx.SegFs = state.x86.segments.fs;
  ctx.SegGs = state.x86.segments.gs;

  // Update control registers
  ctx.Eip = state.x86.gp.eip;
  ctx.EFlags = state.x86.gp.eflags;

  // Update x87 FPU registers
  if (ctx.ContextFlags & CONTEXT_FLOATING_POINT) {
    ctx.FloatSave.ControlWord = state.x86.fpu.fctrl;
    ctx.FloatSave.StatusWord = state.x86.fpu.fstat;
    ctx.FloatSave.TagWord = state.x86.fpu.ftag;
    ctx.FloatSave.ErrorOpcode = state.x86.fpu.fop;
    ctx.FloatSave.ErrorSelector = state.x86.fpu.fiseg;
    ctx.FloatSave.ErrorOffset = state.x86.fpu.fioff;
    ctx.FloatSave.DataSelector = state.x86.fpu.foseg;
    ctx.FloatSave.DataOffset = state.x86.fpu.fooff;

    for (size_t n = 0; n < 8; n++) {
      memcpy(&ctx.FloatSave.RegisterArea[n * 10], &state.x86.fpu.st[n], 10);
    }
  }

  // Update SSE registers if available
  if (ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS) {
    uint8_t *extRegs = (uint8_t *)ctx.ExtendedRegisters;

    *(uint32_t *)(extRegs + 24) = state.x86.sse.mxcsr;

    for (size_t n = 0; n < 8; n++) {
      memcpy(extRegs + 160 + n * 16, &state.x86.sse.xmm[n], 16);
    }
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
