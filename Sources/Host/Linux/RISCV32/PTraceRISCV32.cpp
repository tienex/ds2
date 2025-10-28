//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/RISCV32/CPUState.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace Linux {

using namespace Architecture::RISCV32;

struct user_regs_struct {
  uint32_t pc;
  uint32_t ra;
  uint32_t sp;
  uint32_t gp;
  uint32_t tp;
  uint32_t t0;
  uint32_t t1;
  uint32_t t2;
  uint32_t s0;
  uint32_t s1;
  uint32_t a0;
  uint32_t a1;
  uint32_t a2;
  uint32_t a3;
  uint32_t a4;
  uint32_t a5;
  uint32_t a6;
  uint32_t a7;
  uint32_t s2;
  uint32_t s3;
  uint32_t s4;
  uint32_t s5;
  uint32_t s6;
  uint32_t s7;
  uint32_t s8;
  uint32_t s9;
  uint32_t s10;
  uint32_t s11;
  uint32_t t3;
  uint32_t t4;
  uint32_t t5;
  uint32_t t6;
};

struct user_fpregs_struct {
  uint64_t f[32];   // FP registers (64-bit for D extension)
  uint32_t fcsr;    // FP control/status
};

ErrorCode PTrace::readGPRegisters(ProcessThreadId const &ptid,
                                   Architecture::GPRegisterValueVector &values) {
  ProcessId pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  struct user_regs_struct gprs;
  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();

  // x0 is always 0 (hardwired)
  values.push_back(Architecture::GPRegisterValue{
      0, Architecture::GPRegisterValue::k32bits, 0});

  // Map from user_regs_struct to x1-x31
  uint32_t regs_ordered[31] = {
      gprs.ra,  gprs.sp,  gprs.gp,  gprs.tp,  gprs.t0,  gprs.t1, gprs.t2,
      gprs.s0,  gprs.s1,  gprs.a0,  gprs.a1,  gprs.a2,  gprs.a3, gprs.a4,
      gprs.a5,  gprs.a6,  gprs.a7,  gprs.s2,  gprs.s3,  gprs.s4, gprs.s5,
      gprs.s6,  gprs.s7,  gprs.s8,  gprs.s9,  gprs.s10, gprs.s11, gprs.t3,
      gprs.t4,  gprs.t5,  gprs.t6};

  for (int i = 0; i < 31; i++) {
    values.push_back(Architecture::GPRegisterValue{
        static_cast<uint32_t>(i + 1), Architecture::GPRegisterValue::k32bits,
        regs_ordered[i]});
  }

  // Add PC (register 32)
  values.push_back(Architecture::GPRegisterValue{
      32, Architecture::GPRegisterValue::k32bits, gprs.pc});

  return kSuccess;
}

ErrorCode PTrace::writeGPRegisters(ProcessThreadId const &ptid,
                                    Architecture::GPRegisterValueVector const &values) {
  ProcessId pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Read current state first
  struct user_regs_struct gprs;
  struct iovec iov;
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();

  // Update registers from values
  for (auto const &value : values) {
    uint32_t val = value.value.u32;
    switch (value.regno) {
    case 0:
      break; // x0 is hardwired to zero, ignore writes
    case 1:
      gprs.ra = val;
      break;
    case 2:
      gprs.sp = val;
      break;
    case 3:
      gprs.gp = val;
      break;
    case 4:
      gprs.tp = val;
      break;
    case 5:
      gprs.t0 = val;
      break;
    case 6:
      gprs.t1 = val;
      break;
    case 7:
      gprs.t2 = val;
      break;
    case 8:
      gprs.s0 = val;
      break;
    case 9:
      gprs.s1 = val;
      break;
    case 10:
      gprs.a0 = val;
      break;
    case 11:
      gprs.a1 = val;
      break;
    case 12:
      gprs.a2 = val;
      break;
    case 13:
      gprs.a3 = val;
      break;
    case 14:
      gprs.a4 = val;
      break;
    case 15:
      gprs.a5 = val;
      break;
    case 16:
      gprs.a6 = val;
      break;
    case 17:
      gprs.a7 = val;
      break;
    case 18:
      gprs.s2 = val;
      break;
    case 19:
      gprs.s3 = val;
      break;
    case 20:
      gprs.s4 = val;
      break;
    case 21:
      gprs.s5 = val;
      break;
    case 22:
      gprs.s6 = val;
      break;
    case 23:
      gprs.s7 = val;
      break;
    case 24:
      gprs.s8 = val;
      break;
    case 25:
      gprs.s9 = val;
      break;
    case 26:
      gprs.s10 = val;
      break;
    case 27:
      gprs.s11 = val;
      break;
    case 28:
      gprs.t3 = val;
      break;
    case 29:
      gprs.t4 = val;
      break;
    case 30:
      gprs.t5 = val;
      break;
    case 31:
      gprs.t6 = val;
      break;
    case 32:
      gprs.pc = val;
      break;
    }
  }

  // Write back
  iov.iov_base = &gprs;
  iov.iov_len = sizeof(gprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::readFPRegisters(ProcessThreadId const &ptid,
                                   Architecture::FPRegisterValueVector &values) {
  ProcessId pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  struct user_fpregs_struct fprs;
  struct iovec iov;
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0)
    return Platform::TranslateError();

  // Read all 32 FP registers
  for (int i = 0; i < 32; i++) {
    values.push_back(Architecture::FPRegisterValue{
        static_cast<uint32_t>(i), Architecture::FPRegisterValue::kDouble,
        {.f64 = *reinterpret_cast<double *>(&fprs.f[i])}});
  }

  return kSuccess;
}

ErrorCode PTrace::writeFPRegisters(ProcessThreadId const &ptid,
                                    Architecture::FPRegisterValueVector const &values) {
  ProcessId pid;
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = ptid.pid;

  // Read current state first
  struct user_fpregs_struct fprs;
  struct iovec iov;
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0)
    return Platform::TranslateError();

  // Update FP registers
  for (auto const &value : values) {
    if (value.regno < 32) {
      fprs.f[value.regno] =
          *reinterpret_cast<const uint64_t *>(&value.value.f64);
    }
  }

  // Write back
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
