//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PlayStation 3 Cell Broadband Engine - SPU (Synergistic Processing Unit)
// The SPU has 128 general-purpose 128-bit registers and a unique ISA
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Architecture/PowerPC/CellSPU.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

// Cell SPU context structure for Linux
// SPU contexts are managed differently than traditional threads
struct spu_context_regs {
  uint32_t gpr[128][4];  // 128 general-purpose 128-bit registers
  uint32_t npc;          // Next Program Counter
  uint32_t fpscr;        // Floating-Point Status and Control Register
  uint32_t decrementer;
  uint32_t event_mask;
  uint32_t event_status;
  uint32_t signal1;
  uint32_t signal2;
  uint32_t mb_stat;      // Mailbox status
  uint32_t lslr;         // Local Store Limit Register
};

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // SPU context is accessed via special ptrace requests
  // Linux provides NT_SPU_* note types for SPU register access
  struct iovec iov;
  struct spu_context_regs regs;

  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  // Read SPU registers via PTRACE_GETREGSET
  // Note: This requires SPU-specific kernel support (CONFIG_SPU_FS)
  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_SPU, &iov) < 0)
    return Platform::TranslateError();

  // Copy all 128 general-purpose 128-bit registers
  for (size_t n = 0; n < 128; n++) {
    for (size_t i = 0; i < 4; i++) {
      state.cell_spu.gpr[n].word[i] = regs.gpr[n][i];
    }
  }

  // Copy special registers
  state.cell_spu.npc = regs.npc;
  state.cell_spu.fpscr = regs.fpscr;
  state.cell_spu.decrementer = regs.decrementer;
  state.cell_spu.event_mask = regs.event_mask;
  state.cell_spu.event_status = regs.event_status;
  state.cell_spu.signal1 = regs.signal1;
  state.cell_spu.signal2 = regs.signal2;
  state.cell_spu.mb_stat = regs.mb_stat;
  state.cell_spu.lslr = regs.lslr;

  // Read SPU program counter from separate location
  // The PC is the current instruction being executed
  uint32_t pc;
  iov.iov_base = &pc;
  iov.iov_len = sizeof(pc);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_SPU_PC, &iov) == 0) {
    state.cell_spu.pc = pc;
  }

  // Read MFC (Memory Flow Controller) command queue if available
  // The MFC handles DMA transfers for the SPU
  uint32_t mfc_cq[16];
  iov.iov_base = &mfc_cq;
  iov.iov_len = sizeof(mfc_cq);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_SPU_MFC, &iov) == 0) {
    for (size_t n = 0; n < 16; n++) {
      state.cell_spu.mfc_cq[n] = mfc_cq[n];
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  // Prepare SPU register structure
  struct spu_context_regs regs;

  for (size_t n = 0; n < 128; n++) {
    for (size_t i = 0; i < 4; i++) {
      regs.gpr[n][i] = state.cell_spu.gpr[n].word[i];
    }
  }

  regs.npc = state.cell_spu.npc;
  regs.fpscr = state.cell_spu.fpscr;
  regs.decrementer = state.cell_spu.decrementer;
  regs.event_mask = state.cell_spu.event_mask;
  regs.event_status = state.cell_spu.event_status;
  regs.signal1 = state.cell_spu.signal1;
  regs.signal2 = state.cell_spu.signal2;
  regs.mb_stat = state.cell_spu.mb_stat;
  regs.lslr = state.cell_spu.lslr;

  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_SPU, &iov) < 0)
    return Platform::TranslateError();

  // Write SPU program counter
  uint32_t pc = state.cell_spu.pc;
  iov.iov_base = &pc;
  iov.iov_len = sizeof(pc);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_SPU_PC, &iov) < 0) {
    // Non-fatal - some kernels may not support separate PC update
  }

  // Write MFC command queue
  uint32_t mfc_cq[16];
  for (size_t n = 0; n < 16; n++) {
    mfc_cq[n] = state.cell_spu.mfc_cq[n];
  }

  iov.iov_base = &mfc_cq;
  iov.iov_len = sizeof(mfc_cq);

  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_SPU_MFC, &iov) < 0) {
    // Non-fatal
  }

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
