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
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace Linux {

using namespace Architecture::SuperH;

struct user_regs_struct {
  uint32_t regs[16];    // r0-r15
  uint32_t pc;
  uint32_t pr;
  uint32_t sr;
  uint32_t gbr;
  uint32_t mach;
  uint32_t macl;
  uint32_t vbr;
};

struct user_fpregs_struct {
  uint32_t fr[16];      // FR0-FR15
  uint32_t xf[16];      // XF0-XF15 (banked)
  uint32_t fpscr;
  uint32_t fpul;
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

  // Read all 16 general purpose registers (r0-r15)
  for (int i = 0; i < 16; i++) {
    values.push_back(Architecture::GPRegisterValue{
        static_cast<uint32_t>(i), Architecture::GPRegisterValue::k32bits,
        gprs.regs[i]});
  }

  // Add special registers
  values.push_back(Architecture::GPRegisterValue{
      16, Architecture::GPRegisterValue::k32bits, gprs.pc}); // PC

  values.push_back(Architecture::GPRegisterValue{
      17, Architecture::GPRegisterValue::k32bits, gprs.pr}); // PR

  values.push_back(Architecture::GPRegisterValue{
      18, Architecture::GPRegisterValue::k32bits, gprs.sr}); // SR

  values.push_back(Architecture::GPRegisterValue{
      19, Architecture::GPRegisterValue::k32bits, gprs.gbr}); // GBR

  values.push_back(Architecture::GPRegisterValue{
      20, Architecture::GPRegisterValue::k32bits, gprs.mach}); // MACH

  values.push_back(Architecture::GPRegisterValue{
      21, Architecture::GPRegisterValue::k32bits, gprs.macl}); // MACL

  values.push_back(Architecture::GPRegisterValue{
      22, Architecture::GPRegisterValue::k32bits, gprs.vbr}); // VBR

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
    if (value.regno < 16) {
      gprs.regs[value.regno] = val;
    } else if (value.regno == 16) {
      gprs.pc = val;
    } else if (value.regno == 17) {
      gprs.pr = val;
    } else if (value.regno == 18) {
      gprs.sr = val;
    } else if (value.regno == 19) {
      gprs.gbr = val;
    } else if (value.regno == 20) {
      gprs.mach = val;
    } else if (value.regno == 21) {
      gprs.macl = val;
    } else if (value.regno == 22) {
      gprs.vbr = val;
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

  // Read FR0-FR15 (bank 0)
  for (int i = 0; i < 16; i++) {
    values.push_back(Architecture::FPRegisterValue{
        static_cast<uint32_t>(i), Architecture::FPRegisterValue::kFloat,
        {.f32 = *reinterpret_cast<float *>(&fprs.fr[i])}});
  }

  // Read XF0-XF15 (bank 1)
  for (int i = 0; i < 16; i++) {
    values.push_back(Architecture::FPRegisterValue{
        static_cast<uint32_t>(i + 16), Architecture::FPRegisterValue::kFloat,
        {.f32 = *reinterpret_cast<float *>(&fprs.xf[i])}});
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
    if (value.regno < 16) {
      fprs.fr[value.regno] =
          *reinterpret_cast<const uint32_t *>(&value.value.f32);
    } else if (value.regno < 32) {
      fprs.xf[value.regno - 16] =
          *reinterpret_cast<const uint32_t *>(&value.value.f32);
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
