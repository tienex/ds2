//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>

// OS/2 debugging structures (from bsedos.h)
typedef struct _CONTEXT {
  ULONG ctx_flags;
  ULONG ctx_fs;
  ULONG ctx_gs;
  ULONG ctx_es;
  ULONG ctx_ds;
  ULONG ctx_edi;
  ULONG ctx_esi;
  ULONG ctx_ebp;
  ULONG ctx_esp;
  ULONG ctx_ebx;
  ULONG ctx_edx;
  ULONG ctx_ecx;
  ULONG ctx_eax;
  ULONG ctx_eip;
  ULONG ctx_cs;
  ULONG ctx_eflags;
  ULONG ctx_ss;
} CONTEXTRECORD;

namespace ds2 {
namespace Host {
namespace OS2 {

using namespace Architecture::X86;

ErrorCode ReadCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       Architecture::CPUState &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  CONTEXTRECORD ctx;
  ULONG cbBuf = sizeof(CONTEXTRECORD);

  // Get thread context using DosDebug
  ULONG ulCmd = DBG_C_ReadReg;
  APIRET rc = DosDebug(&ulCmd, ptid.tid, (PVOID)&ctx, &cbBuf);

  if (rc != NO_ERROR) {
    errno = rc;
    return Platform::TranslateError();
  }

  // Map OS/2 context to our CPUState
  state.gp.eax = ctx.ctx_eax;
  state.gp.ebx = ctx.ctx_ebx;
  state.gp.ecx = ctx.ctx_ecx;
  state.gp.edx = ctx.ctx_edx;
  state.gp.esi = ctx.ctx_esi;
  state.gp.edi = ctx.ctx_edi;
  state.gp.ebp = ctx.ctx_ebp;
  state.gp.esp = ctx.ctx_esp;
  state.gp.eip = ctx.ctx_eip;
  state.gp.eflags = ctx.ctx_eflags;

  state.gp.cs = ctx.ctx_cs;
  state.gp.ss = ctx.ctx_ss;
  state.gp.ds = ctx.ctx_ds;
  state.gp.es = ctx.ctx_es;
  state.gp.fs = ctx.ctx_fs;
  state.gp.gs = ctx.ctx_gs;

  return kSuccess;
}

ErrorCode WriteCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                        Architecture::CPUState const &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  CONTEXTRECORD ctx;

  // Map our CPUState to OS/2 context
  ctx.ctx_eax = state.gp.eax;
  ctx.ctx_ebx = state.gp.ebx;
  ctx.ctx_ecx = state.gp.ecx;
  ctx.ctx_edx = state.gp.edx;
  ctx.ctx_esi = state.gp.esi;
  ctx.ctx_edi = state.gp.edi;
  ctx.ctx_ebp = state.gp.ebp;
  ctx.ctx_esp = state.gp.esp;
  ctx.ctx_eip = state.gp.eip;
  ctx.ctx_eflags = state.gp.eflags;

  ctx.ctx_cs = state.gp.cs;
  ctx.ctx_ss = state.gp.ss;
  ctx.ctx_ds = state.gp.ds;
  ctx.ctx_es = state.gp.es;
  ctx.ctx_fs = state.gp.fs;
  ctx.ctx_gs = state.gp.gs;

  ctx.ctx_flags = 0xFFFFFFFF; // Update all registers

  // Set thread context using DosDebug
  ULONG cbBuf = sizeof(CONTEXTRECORD);
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
