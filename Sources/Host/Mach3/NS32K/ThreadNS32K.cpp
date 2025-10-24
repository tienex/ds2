//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Mach3/Thread.h"
#include "DebugServer2/Architecture/NS32K/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/thread_status.h>

using ds2::Host::Mach3::Thread;
using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace Mach3 {

ErrorCode Thread::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  //
  // Mach3 NS32K thread state
  // Use thread_get_state() with NS32K_THREAD_STATE
  //
  thread_t thread = ptid.tid;

  //
  // Get general-purpose register state
  //
  struct ns32k_thread_state {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7;  // General registers
    uint32_t fp;                               // Frame pointer
    uint32_t sp;                               // Stack pointer
    uint32_t sb;                               // Static base
    uint32_t pc;                               // Program counter
    uint32_t psr;                              // Processor status register
    uint32_t mod;                              // Module register
  } gp_state;

  unsigned int count = sizeof(gp_state) / sizeof(int);
  kern_return_t kr;

  kr = thread_get_state(thread, NS32K_THREAD_STATE,
                        (thread_state_t)&gp_state, &count);
  if (kr != KERN_SUCCESS) {
    return Platform::TranslateError();
  }

  //
  // Copy general-purpose registers
  //
  state.gp.r0 = gp_state.r0;
  state.gp.r1 = gp_state.r1;
  state.gp.r2 = gp_state.r2;
  state.gp.r3 = gp_state.r3;
  state.gp.r4 = gp_state.r4;
  state.gp.r5 = gp_state.r5;
  state.gp.r6 = gp_state.r6;
  state.gp.r7 = gp_state.r7;

  //
  // Copy special registers
  //
  state.special.fp = gp_state.fp;
  state.special.sp = gp_state.sp;
  state.special.sb = gp_state.sb;
  state.special.pc = gp_state.pc;
  state.special.psr = gp_state.psr;
  state.special.mod = gp_state.mod;

  //
  // Copy PSR bit fields
  //
  std::memcpy(&state.psr_flags, &gp_state.psr, sizeof(state.psr_flags));

  //
  // Get floating-point register state
  //
  struct ns32k_float_state {
    uint32_t freg[8];    // 8 FPU registers (single precision)
    uint32_t fsr;        // FPU status register
  } fp_state;

  count = sizeof(fp_state) / sizeof(int);
  kr = thread_get_state(thread, NS32K_FLOAT_STATE,
                        (thread_state_t)&fp_state, &count);
  if (kr == KERN_SUCCESS) {
    for (int i = 0; i < 8; i++) {
      state.fpu.raw32[i] = fp_state.freg[i];
    }

    // Decode FSR bit fields
    state.fsr.rm = fp_state.fsr & 0x3;
    state.fsr.uf = (fp_state.fsr >> 5) & 1;
    state.fsr.if_ = (fp_state.fsr >> 6) & 1;
    state.fsr.tt = (fp_state.fsr >> 7) & 1;
  }

  return kSuccess;
}

ErrorCode Thread::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  //
  // Mach3 NS32K thread state
  // Use thread_set_state() with NS32K_THREAD_STATE
  //
  thread_t thread = ptid.tid;

  //
  // Set general-purpose register state
  //
  struct ns32k_thread_state {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint32_t fp, sp, sb, pc, psr, mod;
  } gp_state;

  gp_state.r0 = state.gp.r0;
  gp_state.r1 = state.gp.r1;
  gp_state.r2 = state.gp.r2;
  gp_state.r3 = state.gp.r3;
  gp_state.r4 = state.gp.r4;
  gp_state.r5 = state.gp.r5;
  gp_state.r6 = state.gp.r6;
  gp_state.r7 = state.gp.r7;
  gp_state.fp = state.special.fp;
  gp_state.sp = state.special.sp;
  gp_state.sb = state.special.sb;
  gp_state.pc = state.special.pc;
  gp_state.psr = state.special.psr;
  gp_state.mod = state.special.mod;

  unsigned int count = sizeof(gp_state) / sizeof(int);
  kern_return_t kr;

  kr = thread_set_state(thread, NS32K_THREAD_STATE,
                        (thread_state_t)&gp_state, count);
  if (kr != KERN_SUCCESS) {
    return Platform::TranslateError();
  }

  //
  // Set floating-point register state
  //
  struct ns32k_float_state {
    uint32_t freg[8];
    uint32_t fsr;
  } fp_state;

  for (int i = 0; i < 8; i++) {
    fp_state.freg[i] = state.fpu.raw32[i];
  }

  // Encode FSR from bit fields
  fp_state.fsr = (state.fsr.rm & 0x3) |
                 ((state.fsr.uf & 1) << 5) |
                 ((state.fsr.if_ & 1) << 6) |
                 ((state.fsr.tt & 1) << 7);

  count = sizeof(fp_state) / sizeof(int);
  kr = thread_set_state(thread, NS32K_FLOAT_STATE,
                        (thread_state_t)&fp_state, count);
  if (kr != KERN_SUCCESS) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

} // namespace Mach3
} // namespace Host
} // namespace ds2
