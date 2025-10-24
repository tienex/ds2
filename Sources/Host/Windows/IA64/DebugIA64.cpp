//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows IA-64 (Itanium) Debugging Support
// Supported on Windows Server 2003/2008/2008 R2 for Itanium
//

#include "DebugServer2/Host/Windows/Debug.h"
#include "DebugServer2/Architecture/IA64/CPUState.h"
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

  // Windows IA-64 CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT |
                     CONTEXT_DEBUG_REGISTERS | CONTEXT_CONTROL;

  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  CloseHandle(hThread);

  //
  // Copy general-purpose registers (IntGp-IntT21, IntNats)
  // Windows CONTEXT has IntGp, IntT0-IntT21, IntS0-IntS3, IntSp, IntTeb, IntV0
  //
  // Map Windows IA-64 register names to standard GR0-GR127
  //
  state.gp.regs[0] = 0; // GR0 is hardwired to zero
  state.gp.regs[1] = ctx.IntGp;   // Global pointer
  state.gp.regs[2] = ctx.IntT0;
  state.gp.regs[3] = ctx.IntT1;
  for (int i = 0; i < 20; i++) {
    state.gp.regs[4 + i] = ctx.IntT2 + i * sizeof(ULONGLONG);
  }
  state.gp.regs[12] = ctx.IntSp;  // Stack pointer
  state.gp.regs[13] = ctx.IntTeb; // Thread environment block

  // Copy return value and scratch registers
  state.gp.regs[8] = ctx.IntV0;

  // Copy static registers (S0-S3)
  for (int i = 0; i < 4; i++) {
    state.gp.regs[4 + i] = ctx.IntS0 + i * sizeof(ULONGLONG);
  }

  //
  // Copy branch registers (BrRp, BrT0, BrT1)
  //
  state.br[0] = ctx.BrRp;  // Return pointer (BR0)
  state.br[1] = ctx.BrT0;
  state.br[2] = ctx.BrT1;

  //
  // Copy predicate registers (Preds)
  //
  state.pr = ctx.Preds;

  //
  // Copy instruction pointer (StIIP)
  //
  state.ip = ctx.StIIP;

  //
  // Copy current frame marker (StIFS)
  //
  std::memcpy(&state.cfm, &ctx.StIFS, sizeof(state.cfm));

  //
  // Copy application registers
  //
  state.ar.rsc = ctx.RsRSC;        // Register Stack Configuration
  state.ar.bsp = ctx.RsBSP;        // Backing Store Pointer
  state.ar.bspstore = ctx.RsBSPSTORE; // BSP Store
  state.ar.rnat = ctx.RsRNAT;      // RSE NaT Collection
  state.ar.pfs = ctx.StFPSR;       // Previous Function State
  state.ar.unat = ctx.IntNats;     // User NaT Collection
  state.ar.ccv = 0;                // Not directly exposed
  state.ar.fpsr = ctx.StFPSR;      // Floating-Point Status Register

  //
  // Copy processor status register (StIPSR)
  //
  std::memcpy(&state.psr, &ctx.StIPSR, sizeof(state.psr));

  //
  // Copy floating-point registers (FltF0-FltF127)
  // Windows exposes FltF6-FltF15, FltF16-FltF31, FltF32-FltF127
  //
  state.fpr[0].significand = 0;
  state.fpr[0].exponent = 0;
  state.fpr[0].sign = 0; // FR0 = +0.0 (hardwired)

  state.fpr[1].significand = 1ULL << 63;
  state.fpr[1].exponent = 0xFFFF;
  state.fpr[1].sign = 0; // FR1 = +1.0 (hardwired)

  // Copy float registers from CONTEXT
  // FltF6-FltF15 (preserved)
  for (int i = 6; i <= 15; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF6 + (i - 6) * sizeof(FLOAT128));
    state.fpr[i].significand = f128->LowPart;
    state.fpr[i].exponent = (f128->HighPart & 0x1FFFF);
    state.fpr[i].sign = (f128->HighPart >> 17) & 1;
  }

  // FltF16-FltF31 (scratch)
  for (int i = 16; i <= 31; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF16 + (i - 16) * sizeof(FLOAT128));
    state.fpr[i].significand = f128->LowPart;
    state.fpr[i].exponent = (f128->HighPart & 0x1FFFF);
    state.fpr[i].sign = (f128->HighPart >> 17) & 1;
  }

  // FltF32-FltF127 (stacked/rotating)
  for (int i = 32; i < 128; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF32 + (i - 32) * sizeof(FLOAT128));
    state.fpr[i].significand = f128->LowPart;
    state.fpr[i].exponent = (f128->HighPart & 0x1FFFF);
    state.fpr[i].sign = (f128->HighPart >> 17) & 1;
  }

  //
  // Copy NaT bits (IntNats)
  //
  state.nat.nat_low = ctx.IntNats & 0xFFFFFFFFFFFFFFFFULL;
  state.nat.nat_high = (ctx.IntNats >> 64) & 0xFFFFFFFFFFFFFFFFULL;

  //
  // Copy debug registers (DbI0-DbI7, DbD0-DbD7)
  //
  for (int i = 0; i < 8; i++) {
    state.debug.ibr[i].addr = ctx.DbI0 + i * sizeof(ULONGLONG) * 2;
    state.debug.ibr[i].mask = ctx.DbI0 + (i * 2 + 1) * sizeof(ULONGLONG);
    state.debug.dbr[i].addr = ctx.DbD0 + i * sizeof(ULONGLONG) * 2;
    state.debug.dbr[i].mask = ctx.DbD0 + (i * 2 + 1) * sizeof(ULONGLONG);
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

  // Prepare Windows IA-64 CONTEXT structure
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT |
                     CONTEXT_DEBUG_REGISTERS | CONTEXT_CONTROL;

  // First, get current context to preserve non-modified fields
  if (!GetThreadContext(hThread, &ctx)) {
    CloseHandle(hThread);
    return Platform::TranslateError();
  }

  //
  // Update general-purpose registers
  //
  ctx.IntGp = state.gp.regs[1];
  ctx.IntT0 = state.gp.regs[2];
  ctx.IntT1 = state.gp.regs[3];
  for (int i = 0; i < 20; i++) {
    *(&ctx.IntT2 + i) = state.gp.regs[4 + i];
  }
  ctx.IntSp = state.gp.regs[12];
  ctx.IntTeb = state.gp.regs[13];
  ctx.IntV0 = state.gp.regs[8];

  for (int i = 0; i < 4; i++) {
    *(&ctx.IntS0 + i) = state.gp.regs[4 + i];
  }

  //
  // Update branch registers
  //
  ctx.BrRp = state.br[0];
  ctx.BrT0 = state.br[1];
  ctx.BrT1 = state.br[2];

  //
  // Update predicate registers
  //
  ctx.Preds = state.pr;

  //
  // Update instruction pointer
  //
  ctx.StIIP = state.ip;

  //
  // Update current frame marker
  //
  std::memcpy(&ctx.StIFS, &state.cfm, sizeof(ctx.StIFS));

  //
  // Update application registers
  //
  ctx.RsRSC = state.ar.rsc;
  ctx.RsBSP = state.ar.bsp;
  ctx.RsBSPSTORE = state.ar.bspstore;
  ctx.RsRNAT = state.ar.rnat;
  ctx.StFPSR = state.ar.fpsr;
  ctx.IntNats = state.ar.unat;

  //
  // Update processor status register
  //
  std::memcpy(&ctx.StIPSR, &state.psr, sizeof(ctx.StIPSR));

  //
  // Update floating-point registers
  //
  // FltF6-FltF15
  for (int i = 6; i <= 15; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF6 + (i - 6) * sizeof(FLOAT128));
    f128->LowPart = state.fpr[i].significand;
    f128->HighPart = (state.fpr[i].exponent & 0x1FFFF) |
                     ((state.fpr[i].sign & 1) << 17);
  }

  // FltF16-FltF31
  for (int i = 16; i <= 31; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF16 + (i - 16) * sizeof(FLOAT128));
    f128->LowPart = state.fpr[i].significand;
    f128->HighPart = (state.fpr[i].exponent & 0x1FFFF) |
                     ((state.fpr[i].sign & 1) << 17);
  }

  // FltF32-FltF127
  for (int i = 32; i < 128; i++) {
    FLOAT128 *f128 = (FLOAT128 *)(&ctx.FltF32 + (i - 32) * sizeof(FLOAT128));
    f128->LowPart = state.fpr[i].significand;
    f128->HighPart = (state.fpr[i].exponent & 0x1FFFF) |
                     ((state.fpr[i].sign & 1) << 17);
  }

  //
  // Update NaT bits
  //
  ctx.IntNats = state.nat.nat_low | (static_cast<__uint128_t>(state.nat.nat_high) << 64);

  //
  // Update debug registers
  //
  for (int i = 0; i < 8; i++) {
    *(&ctx.DbI0 + i * 2) = state.debug.ibr[i].addr;
    *(&ctx.DbI0 + i * 2 + 1) = state.debug.ibr[i].mask;
    *(&ctx.DbD0 + i * 2) = state.debug.dbr[i].addr;
    *(&ctx.DbD0 + i * 2 + 1) = state.debug.dbr[i].mask;
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
