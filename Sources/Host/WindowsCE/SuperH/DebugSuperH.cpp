//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/SuperH/CPUState.h"
#include "DebugServer2/Host/Windows/Debug.h"
#include "DebugServer2/Host/Platform.h"

#include <windows.h>

#define super Debug

namespace ds2 {
namespace Host {
namespace Windows {

using namespace Architecture::SuperH;

// Windows CE CONTEXT structure for SuperH (SHx)
// This is a simplified version based on Windows CE SDK
#ifndef CONTEXT_SH4
#define CONTEXT_SH4 0x00040000
#define CONTEXT_CONTROL (CONTEXT_SH4 | 0x00000001L)
#define CONTEXT_INTEGER (CONTEXT_SH4 | 0x00000002L)
#define CONTEXT_FLOATING_POINT (CONTEXT_SH4 | 0x00000004L)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)

typedef struct _CONTEXT_SH {
  DWORD ContextFlags;

  // General purpose registers
  DWORD R0;
  DWORD R1;
  DWORD R2;
  DWORD R3;
  DWORD R4;
  DWORD R5;
  DWORD R6;
  DWORD R7;
  DWORD R8;
  DWORD R9;
  DWORD R10;
  DWORD R11;
  DWORD R12;
  DWORD R13;
  DWORD R14;
  DWORD R15;

  // Control registers
  DWORD PR;
  DWORD MACH;
  DWORD MACL;
  DWORD GBR;
  DWORD SR;
  DWORD PC;

  // Floating point registers (SH-4)
  DWORD Fpscr;
  DWORD Fpul;
  DWORD FRegs[16]; // FR0-FR15
  DWORD xFRegs[16]; // XF0-XF15 (banked)
} CONTEXT_SH;
#endif

ErrorCode Debug::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  HANDLE hThread;
  CONTEXT_SH ctx;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                       ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  memset(&ctx, 0, sizeof(ctx));
  ctx.ContextFlags = CONTEXT_FULL;

  if (!GetThreadContext(hThread, reinterpret_cast<LPCONTEXT>(&ctx))) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  // Map Windows CE CONTEXT to our CPUState
  state.gp.regs[0] = ctx.R0;
  state.gp.regs[1] = ctx.R1;
  state.gp.regs[2] = ctx.R2;
  state.gp.regs[3] = ctx.R3;
  state.gp.regs[4] = ctx.R4;
  state.gp.regs[5] = ctx.R5;
  state.gp.regs[6] = ctx.R6;
  state.gp.regs[7] = ctx.R7;
  state.gp.regs[8] = ctx.R8;
  state.gp.regs[9] = ctx.R9;
  state.gp.regs[10] = ctx.R10;
  state.gp.regs[11] = ctx.R11;
  state.gp.regs[12] = ctx.R12;
  state.gp.regs[13] = ctx.R13;
  state.gp.regs[14] = ctx.R14;
  state.gp.regs[15] = ctx.R15;

  state.special.pc = ctx.PC;
  state.special.pr = ctx.PR;
  state.special.sr = ctx.SR;
  state.special.gbr = ctx.GBR;
  state.special.mach = ctx.MACH;
  state.special.macl = ctx.MACL;

  // Floating point
  state.fpu_ctrl.fpscr = ctx.Fpscr;
  state.fpu_ctrl.fpul = ctx.Fpul;

  for (int i = 0; i < 16; i++) {
    state.fpu.w[i] = ctx.FRegs[i];
    state.fpu.w[i + 16] = ctx.xFRegs[i];
  }

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  HANDLE hThread;
  CONTEXT_SH ctx;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                       ptid.tid);
  if (hThread == NULL)
    return Platform::TranslateError();

  // Read current context first
  memset(&ctx, 0, sizeof(ctx));
  ctx.ContextFlags = CONTEXT_FULL;

  if (!GetThreadContext(hThread, reinterpret_cast<LPCONTEXT>(&ctx))) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  // Update context from our CPUState
  ctx.R0 = state.gp.regs[0];
  ctx.R1 = state.gp.regs[1];
  ctx.R2 = state.gp.regs[2];
  ctx.R3 = state.gp.regs[3];
  ctx.R4 = state.gp.regs[4];
  ctx.R5 = state.gp.regs[5];
  ctx.R6 = state.gp.regs[6];
  ctx.R7 = state.gp.regs[7];
  ctx.R8 = state.gp.regs[8];
  ctx.R9 = state.gp.regs[9];
  ctx.R10 = state.gp.regs[10];
  ctx.R11 = state.gp.regs[11];
  ctx.R12 = state.gp.regs[12];
  ctx.R13 = state.gp.regs[13];
  ctx.R14 = state.gp.regs[14];
  ctx.R15 = state.gp.regs[15];

  ctx.PC = state.special.pc;
  ctx.PR = state.special.pr;
  ctx.SR = state.special.sr;
  ctx.GBR = state.special.gbr;
  ctx.MACH = state.special.mach;
  ctx.MACL = state.special.macl;

  // Floating point
  ctx.Fpscr = state.fpu_ctrl.fpscr;
  ctx.Fpul = state.fpu_ctrl.fpul;

  for (int i = 0; i < 16; i++) {
    ctx.FRegs[i] = state.fpu.w[i];
    ctx.xFRegs[i] = state.fpu.w[i + 16];
  }

  // Write context back
  if (!SetThreadContext(hThread, reinterpret_cast<LPCONTEXT>(&ctx))) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);
  return kSuccess;
}

} // namespace Windows
} // namespace Host
} // namespace ds2
