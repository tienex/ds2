//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Architecture/X86/RegisterCopy.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

struct PTracePrivateData {
  uint8_t breakpointCount;
  uint8_t watchpointCount;
  uint8_t maxWatchpointSize;

  PTracePrivateData()
      : breakpointCount(0), watchpointCount(0), maxWatchpointSize(0) {}
};

void PTrace::initCPUState(ProcessId pid) {
  if (_privateData != nullptr)
    return;

  _privateData = new PTracePrivateData;
}

void PTrace::doneCPUState() { delete _privateData; }

static inline void user_to_state32(ds2::Architecture::X86::CPUState &state,
                                   user_fpxregs_struct const &user) {
  // X87 State
  state.x87.fstw = user.swd;
  state.x87.fctw = user.cwd;
  state.x87.ftag = user.twd;
  state.x87.fop = user.fop;
  state.x87.fiseg = user.fcs;
  state.x87.fioff = user.fip;
  state.x87.foseg = user.fos;
  state.x87.fooff = user.foo;

  auto st_space = reinterpret_cast<uint8_t const *>(user.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * x87Size, x87Size);
  }

  // SSE state
  state.sse.mxcsr = user.mxcsr;
  state.sse.mxcsrmask = user.reserved;
  auto xmm_space = reinterpret_cast<uint8_t const *>(user.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < array_sizeof(state.sse.regs); n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseSize, sseSize);
  }
}

static inline void
state32_to_user(user_fpxregs_struct &user,
                ds2::Architecture::X86::CPUState const &state) {
  // X87 State
  user.swd = state.x87.fstw;
  user.cwd = state.x87.fctw;
  user.twd = state.x87.ftag;
  user.fop = state.x87.fop;
  user.fcs = state.x87.fiseg;
  user.fip = state.x87.fioff;
  user.fos = state.x87.foseg;
  user.foo = state.x87.fooff;

  auto st_space = reinterpret_cast<uint8_t *>(user.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(st_space + n * x87Size, state.x87.regs[n].bytes, x87Size);
  }

  // SSE state
  user.mxcsr = state.sse.mxcsr;
  user.reserved = state.sse.mxcsrmask;
  auto xmm_space = reinterpret_cast<uint8_t *>(user.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < array_sizeof(state.sse.regs); n++) {
    memcpy(xmm_space + n * sseSize, &state.sse.regs[n], sseSize);
  }
}

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  //
  // Read GPRs
  //
  user_regs_struct gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  Architecture::X86::user_to_state32(state, gprs);

  //
  // Read X87 and SSE state
  //
  user_fpxregs_struct fxrs;
  if (wrapPtrace(PTRACE_GETFPXREGS, pid, nullptr, &fxrs) == 0) {
    user_to_state32(state, fxrs);
  } else {
    //
    // Try reading only X87
    //
    user_fpregs_struct fprs;
    if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fprs) == 0) {
      state.x87.fstw = fprs.swd;
      state.x87.fctw = fprs.cwd;
      state.x87.ftag = fprs.twd;
      state.x87.fiseg = fprs.fcs;
      state.x87.fioff = fprs.fip;
      state.x87.foseg = fprs.fos;
      state.x87.fooff = fprs.foo;

      auto st_space = reinterpret_cast<uint8_t const *>(fprs.st_space);
      for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
        memcpy(state.x87.regs[n].bytes, st_space + n * 10,
               sizeof(state.x87.regs[n].bytes));
      }
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  //
  // Write GPRs
  //
  user_regs_struct gprs;
  Architecture::X86::state32_to_user(gprs, state);

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  //
  // Write X87 and SSE state
  //
  user_fpxregs_struct fxrs;
  state32_to_user(fxrs, state);

  if (wrapPtrace(PTRACE_SETFPXREGS, pid, nullptr, &fxrs) < 0) {
    //
    // Try writing only X87
    //
    user_fpregs_struct fprs;
    fprs.swd = state.x87.fstw;
    fprs.cwd = state.x87.fctw;
    fprs.twd = state.x87.ftag;
    fprs.fcs = state.x87.fiseg;
    fprs.fip = state.x87.fioff;
    fprs.fos = state.x87.foseg;
    fprs.foo = state.x87.fooff;

    auto st_space = reinterpret_cast<uint8_t *>(fprs.st_space);
    for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
      memcpy(st_space + n * 10, state.x87.regs[n].bytes,
             sizeof(state.x87.regs[n].bytes));
    }
    wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fprs);
  }

  return kSuccess;
}
}
}
}
