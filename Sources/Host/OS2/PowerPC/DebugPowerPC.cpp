//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>

// OS/2 PowerPC debugging structures
// OS/2 PowerPC was experimental and used similar debugging APIs

typedef struct _PPC_CONTEXT {
  ULONG ctx_flags;
  ULONG gpr[32];      // General purpose registers
  ULONG cr;           // Condition register
  ULONG xer;          // Fixed-point exception register
  ULONG lr;           // Link register
  ULONG ctr;          // Count register
  ULONG pc;           // Program counter
  ULONG msr;          // Machine state register
  double fpr[32];     // Floating point registers
  ULONG fpscr;        // FP status and control register
} PPC_CONTEXTRECORD;

namespace ds2 {
namespace Host {
namespace OS2 {

using namespace Architecture::PowerPC;

ErrorCode ReadCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       Architecture::CPUState &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  PPC_CONTEXTRECORD ctx;
  ULONG cbBuf = sizeof(PPC_CONTEXTRECORD);

  // Get thread context using DosDebug
  ULONG ulCmd = DBG_C_ReadReg;
  APIRET rc = DosDebug(&ulCmd, ptid.tid, (PVOID)&ctx, &cbBuf);

  if (rc != NO_ERROR) {
    errno = rc;
    return Platform::TranslateError();
  }

  // Map OS/2 PowerPC context to our CPUState
  for (int i = 0; i < 32; i++) {
    state.gp.regs[i] = ctx.gpr[i];
  }

  state.special.pc = ctx.pc;
  state.special.lr = ctx.lr;
  state.special.ctr = ctx.ctr;
  state.special.cr = ctx.cr;
  state.special.xer = ctx.xer;
  state.special.msr = ctx.msr;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    state.fpu.regs[i] = ctx.fpr[i];
  }
  state.fpu_ctrl.fpscr = ctx.fpscr;

  return kSuccess;
}

ErrorCode WriteCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                        Architecture::CPUState const &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  PPC_CONTEXTRECORD ctx;

  // Map our CPUState to OS/2 PowerPC context
  for (int i = 0; i < 32; i++) {
    ctx.gpr[i] = state.gp.regs[i];
  }

  ctx.pc = state.special.pc;
  ctx.lr = state.special.lr;
  ctx.ctr = state.special.ctr;
  ctx.cr = state.special.cr;
  ctx.xer = state.special.xer;
  ctx.msr = state.special.msr;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    ctx.fpr[i] = state.fpu.regs[i];
  }
  ctx.fpscr = state.fpu_ctrl.fpscr;

  ctx.ctx_flags = 0xFFFFFFFF; // Update all registers

  // Set thread context using DosDebug
  ULONG cbBuf = sizeof(PPC_CONTEXTRECORD);
  ULONG ulCmd = DBG_C_WriteReg;
  APIRET rc = DosDebug(&ulCmd, ptid.tid, (PVOID)&ctx, &cbBuf);

  if (rc != NO_ERROR) {
    errno = rc;
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace OS2
} // namespace Host
} // namespace ds2
