//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/LoongArch32/CPUState.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace Linux {

using namespace Architecture::LoongArch32;

struct user_regs_struct {
  uint32_t regs[32];  // r0-r31
  uint32_t orig_a0;
  uint32_t csr_era;   // PC (Exception Return Address)
  uint32_t csr_badv;  // Bad Virtual Address
  uint32_t csr_crmd;  // Current Mode Info
  uint32_t csr_prmd;  // Previous Mode Info
  uint32_t csr_euen;  // Extended Unit Enable
  uint32_t csr_ecfg;  // Exception Configuration
  uint32_t csr_estat; // Exception Status
  uint32_t __reserved;
};

struct user_fp_struct {
  uint64_t fpr[32];   // 32 FP registers (can hold double)
  uint64_t fcc;       // FP condition codes
  uint32_t fcsr;      // FP control/status
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

  // Read all 32 general purpose registers
  for (int i = 0; i < 32; i++) {
    values.push_back(Architecture::GPRegisterValue{
        static_cast<uint32_t>(i),
        Architecture::GPRegisterValue::k32bits,
        gprs.regs[i]});
  }

  // Add PC
  values.push_back(Architecture::GPRegisterValue{
      32, Architecture::GPRegisterValue::k32bits, gprs.csr_era});

  // Add BADV
  values.push_back(Architecture::GPRegisterValue{
      33, Architecture::GPRegisterValue::k32bits, gprs.csr_badv});

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
    if (value.regno < 32) {
      gprs.regs[value.regno] = value.value.u32;
    } else if (value.regno == 32) {
      gprs.csr_era = value.value.u32; // PC
    } else if (value.regno == 33) {
      gprs.csr_badv = value.value.u32; // BADV
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

  struct user_fp_struct fprs;
  struct iovec iov;
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0)
    return Platform::TranslateError();

  // Read all 32 FP registers
  for (int i = 0; i < 32; i++) {
    values.push_back(Architecture::FPRegisterValue{
        static_cast<uint32_t>(i),
        Architecture::FPRegisterValue::kDouble,
        {.f64 = *reinterpret_cast<double *>(&fprs.fpr[i])}});
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
  struct user_fp_struct fprs;
  struct iovec iov;
  iov.iov_base = &fprs;
  iov.iov_len = sizeof(fprs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) < 0)
    return Platform::TranslateError();

  // Update FP registers
  for (auto const &value : values) {
    if (value.regno < 32) {
      fprs.fpr[value.regno] = *reinterpret_cast<const uint64_t *>(&value.value.f64);
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
