//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows 2000 Alpha64 (AXP64) Thread
// This is for the never-released 64-bit Alpha port of Windows 2000
//

#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

namespace ds2 {
namespace Target {
namespace Windows {

//
// Windows 2000 AXP64 (Alpha 64-bit) Thread Support
//
// Windows 2000 for Alpha was in development but never released.
// It would have used 64-bit Alpha (AXP64) as opposed to the 32-bit
// Alpha32 variant used in Windows NT 3.51-4.0.
//
// The CONTEXT structure for AXP64 would have been similar to the
// 32-bit version but with 64-bit registers.
//

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  CONTEXT context;
  context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  if (!GetThreadContext(_handle, &context)) {
    DS2LOG(Error, "GetThreadContext failed: %lu", GetLastError());
    return kErrorInvalidArgument;
  }

  // Windows AXP64 CONTEXT structure (64-bit Alpha)
  // Based on the Alpha architecture, but using Windows naming conventions

  // Integer registers (64-bit)
  // Windows uses IntV0, IntT0-IntT7, IntS0-IntS5, IntFp, IntA0-IntA5, etc.
  state.alpha.gp.v0 = context.IntV0;      // r0
  state.alpha.gp.t0 = context.IntT0;      // r1
  state.alpha.gp.t1 = context.IntT1;      // r2
  state.alpha.gp.t2 = context.IntT2;      // r3
  state.alpha.gp.t3 = context.IntT3;      // r4
  state.alpha.gp.t4 = context.IntT4;      // r5
  state.alpha.gp.t5 = context.IntT5;      // r6
  state.alpha.gp.t6 = context.IntT6;      // r7
  state.alpha.gp.t7 = context.IntT7;      // r8
  state.alpha.gp.s0 = context.IntS0;      // r9
  state.alpha.gp.s1 = context.IntS1;      // r10
  state.alpha.gp.s2 = context.IntS2;      // r11
  state.alpha.gp.s3 = context.IntS3;      // r12
  state.alpha.gp.s4 = context.IntS4;      // r13
  state.alpha.gp.s5 = context.IntS5;      // r14
  state.alpha.gp.fp = context.IntFp;      // r15
  state.alpha.gp.a0 = context.IntA0;      // r16
  state.alpha.gp.a1 = context.IntA1;      // r17
  state.alpha.gp.a2 = context.IntA2;      // r18
  state.alpha.gp.a3 = context.IntA3;      // r19
  state.alpha.gp.a4 = context.IntA4;      // r20
  state.alpha.gp.a5 = context.IntA5;      // r21
  state.alpha.gp.t8 = context.IntT8;      // r22
  state.alpha.gp.t9 = context.IntT9;      // r23
  state.alpha.gp.t10 = context.IntT10;    // r24
  state.alpha.gp.t11 = context.IntT11;    // r25
  state.alpha.gp.ra = context.IntRa;      // r26
  state.alpha.gp.pv = context.IntT12;     // r27 (PV/T12)
  state.alpha.gp.at = context.IntAt;      // r28
  state.alpha.gp.gp = context.IntGp;      // r29
  state.alpha.gp.sp = context.IntSp;      // r30
  state.alpha.gp.zero = context.IntZero;  // r31 (always 0)

  // Program counter (Fir = Fault Instruction Register)
  state.alpha.pc = context.Fir;

  // Floating-point registers (64-bit IEEE 754)
  // Windows uses FltF0-FltF31
  state.alpha.fp.regs[0] = context.FltF0;
  state.alpha.fp.regs[1] = context.FltF1;
  state.alpha.fp.regs[2] = context.FltF2;
  state.alpha.fp.regs[3] = context.FltF3;
  state.alpha.fp.regs[4] = context.FltF4;
  state.alpha.fp.regs[5] = context.FltF5;
  state.alpha.fp.regs[6] = context.FltF6;
  state.alpha.fp.regs[7] = context.FltF7;
  state.alpha.fp.regs[8] = context.FltF8;
  state.alpha.fp.regs[9] = context.FltF9;
  state.alpha.fp.regs[10] = context.FltF10;
  state.alpha.fp.regs[11] = context.FltF11;
  state.alpha.fp.regs[12] = context.FltF12;
  state.alpha.fp.regs[13] = context.FltF13;
  state.alpha.fp.regs[14] = context.FltF14;
  state.alpha.fp.regs[15] = context.FltF15;
  state.alpha.fp.regs[16] = context.FltF16;
  state.alpha.fp.regs[17] = context.FltF17;
  state.alpha.fp.regs[18] = context.FltF18;
  state.alpha.fp.regs[19] = context.FltF19;
  state.alpha.fp.regs[20] = context.FltF20;
  state.alpha.fp.regs[21] = context.FltF21;
  state.alpha.fp.regs[22] = context.FltF22;
  state.alpha.fp.regs[23] = context.FltF23;
  state.alpha.fp.regs[24] = context.FltF24;
  state.alpha.fp.regs[25] = context.FltF25;
  state.alpha.fp.regs[26] = context.FltF26;
  state.alpha.fp.regs[27] = context.FltF27;
  state.alpha.fp.regs[28] = context.FltF28;
  state.alpha.fp.regs[29] = context.FltF29;
  state.alpha.fp.regs[30] = context.FltF30;
  state.alpha.fp.regs[31] = context.FltF31;

  // Floating-point control register
  state.alpha.fpcr = context.Fpcr;

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  CONTEXT context;
  context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

  // Read current context first
  if (!GetThreadContext(_handle, &context)) {
    DS2LOG(Error, "GetThreadContext failed: %lu", GetLastError());
    return kErrorInvalidArgument;
  }

  // Integer registers
  context.IntV0 = state.alpha.gp.v0;
  context.IntT0 = state.alpha.gp.t0;
  context.IntT1 = state.alpha.gp.t1;
  context.IntT2 = state.alpha.gp.t2;
  context.IntT3 = state.alpha.gp.t3;
  context.IntT4 = state.alpha.gp.t4;
  context.IntT5 = state.alpha.gp.t5;
  context.IntT6 = state.alpha.gp.t6;
  context.IntT7 = state.alpha.gp.t7;
  context.IntS0 = state.alpha.gp.s0;
  context.IntS1 = state.alpha.gp.s1;
  context.IntS2 = state.alpha.gp.s2;
  context.IntS3 = state.alpha.gp.s3;
  context.IntS4 = state.alpha.gp.s4;
  context.IntS5 = state.alpha.gp.s5;
  context.IntFp = state.alpha.gp.fp;
  context.IntA0 = state.alpha.gp.a0;
  context.IntA1 = state.alpha.gp.a1;
  context.IntA2 = state.alpha.gp.a2;
  context.IntA3 = state.alpha.gp.a3;
  context.IntA4 = state.alpha.gp.a4;
  context.IntA5 = state.alpha.gp.a5;
  context.IntT8 = state.alpha.gp.t8;
  context.IntT9 = state.alpha.gp.t9;
  context.IntT10 = state.alpha.gp.t10;
  context.IntT11 = state.alpha.gp.t11;
  context.IntRa = state.alpha.gp.ra;
  context.IntT12 = state.alpha.gp.pv;
  context.IntAt = state.alpha.gp.at;
  context.IntGp = state.alpha.gp.gp;
  context.IntSp = state.alpha.gp.sp;
  context.IntZero = state.alpha.gp.zero;

  // Program counter
  context.Fir = state.alpha.pc;

  // Floating-point registers
  context.FltF0 = state.alpha.fp.regs[0];
  context.FltF1 = state.alpha.fp.regs[1];
  context.FltF2 = state.alpha.fp.regs[2];
  context.FltF3 = state.alpha.fp.regs[3];
  context.FltF4 = state.alpha.fp.regs[4];
  context.FltF5 = state.alpha.fp.regs[5];
  context.FltF6 = state.alpha.fp.regs[6];
  context.FltF7 = state.alpha.fp.regs[7];
  context.FltF8 = state.alpha.fp.regs[8];
  context.FltF9 = state.alpha.fp.regs[9];
  context.FltF10 = state.alpha.fp.regs[10];
  context.FltF11 = state.alpha.fp.regs[11];
  context.FltF12 = state.alpha.fp.regs[12];
  context.FltF13 = state.alpha.fp.regs[13];
  context.FltF14 = state.alpha.fp.regs[14];
  context.FltF15 = state.alpha.fp.regs[15];
  context.FltF16 = state.alpha.fp.regs[16];
  context.FltF17 = state.alpha.fp.regs[17];
  context.FltF18 = state.alpha.fp.regs[18];
  context.FltF19 = state.alpha.fp.regs[19];
  context.FltF20 = state.alpha.fp.regs[20];
  context.FltF21 = state.alpha.fp.regs[21];
  context.FltF22 = state.alpha.fp.regs[22];
  context.FltF23 = state.alpha.fp.regs[23];
  context.FltF24 = state.alpha.fp.regs[24];
  context.FltF25 = state.alpha.fp.regs[25];
  context.FltF26 = state.alpha.fp.regs[26];
  context.FltF27 = state.alpha.fp.regs[27];
  context.FltF28 = state.alpha.fp.regs[28];
  context.FltF29 = state.alpha.fp.regs[29];
  context.FltF30 = state.alpha.fp.regs[30];
  context.FltF31 = state.alpha.fp.regs[31];

  // Floating-point control register
  context.Fpcr = state.alpha.fpcr;

  if (!SetThreadContext(_handle, &context)) {
    DS2LOG(Error, "SetThreadContext failed: %lu", GetLastError());
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

} // namespace Windows
} // namespace Target
} // namespace ds2
