//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PARISC/CPUState.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super PTrace

namespace ds2 {
namespace Host {
namespace Linux {

using namespace Architecture::PARISC;

// Linux PA-RISC user register structure
struct user_regs_struct {
  uint32_t gr[32];      // General registers
  uint32_t sr[8];       // Space registers
  uint32_t iaoq[2];     // Instruction address offset queue
  uint32_t iasq[2];     // Instruction address space queue
  uint32_t sar;         // Shift amount register
  uint32_t iir;         // Interrupt instruction register
  uint32_t isr;         // Interrupt space register
  uint32_t ior;         // Interrupt offset register
  uint32_t ipsw;        // Interrupt PSW
  uint32_t cr27;        // cr27 (thread pointer on Linux)
};

struct user_fp_struct {
  double fpr[32];       // FP registers (64-bit double)
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
        static_cast<uint32_t>(i), Architecture::GPRegisterValue::k32bits,
        gprs.gr[i]});
  }

  // Add space registers (32-39)
  for (int i = 0; i < 8; i++) {
    values.push_back(Architecture::GPRegisterValue{
        static_cast<uint32_t>(32 + i), Architecture::GPRegisterValue::k32bits,
        gprs.sr[i]});
  }

  // Add special registers
  values.push_back(Architecture::GPRegisterValue{
      40, Architecture::GPRegisterValue::k32bits, gprs.iaoq[0]}); // IAOQ head

  values.push_back(Architecture::GPRegisterValue{
      41, Architecture::GPRegisterValue::k32bits, gprs.iaoq[1]}); // IAOQ tail

  values.push_back(Architecture::GPRegisterValue{
      42, Architecture::GPRegisterValue::k32bits, gprs.iasq[0]}); // IASQ head

  values.push_back(Architecture::GPRegisterValue{
      43, Architecture::GPRegisterValue::k32bits, gprs.iasq[1]}); // IASQ tail

  values.push_back(Architecture::GPRegisterValue{
      44, Architecture::GPRegisterValue::k32bits, gprs.sar}); // SAR

  values.push_back(Architecture::GPRegisterValue{
      45, Architecture::GPRegisterValue::k32bits, gprs.ipsw}); // IPSW

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
    
    if (value.regno < 32) {
      gprs.gr[value.regno] = val;
    } else if (value.regno >= 32 && value.regno < 40) {
      gprs.sr[value.regno - 32] = val;
    } else if (value.regno == 40) {
      gprs.iaoq[0] = val;
    } else if (value.regno == 41) {
      gprs.iaoq[1] = val;
    } else if (value.regno == 42) {
      gprs.iasq[0] = val;
    } else if (value.regno == 43) {
      gprs.iasq[1] = val;
    } else if (value.regno == 44) {
      gprs.sar = val;
    } else if (value.regno == 45) {
      gprs.ipsw = val;
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
        static_cast<uint32_t>(i), Architecture::FPRegisterValue::kDouble,
        {.f64 = fprs.fpr[i]}});
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
      fprs.fpr[value.regno] = value.value.f64;
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
