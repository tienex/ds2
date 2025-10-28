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
// FreeBSD port - used in PS3 homebrew and development
//

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Architecture/PowerPC/CellSPU.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <sys/types.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

// FreeBSD Cell SPU register structure
struct spu_regs {
  uint32_t gpr[128][4];  // 128 general-purpose 128-bit registers
  uint32_t pc;           // Program Counter
  uint32_t npc;          // Next Program Counter
  uint32_t fpscr;        // Floating-Point Status and Control Register
  uint32_t decrementer;
  uint32_t event_mask;
  uint32_t event_status;
  uint32_t signal1;
  uint32_t signal2;
  uint32_t mb_stat;      // Mailbox status
  uint32_t mb_data;      // Mailbox data
  uint32_t status;
  uint32_t lslr;         // Local Store Limit Register
};

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  // Read SPU registers via FreeBSD-specific ptrace request
  // FreeBSD uses PT_GETSPUREGS for Cell SPU register access
  struct spu_regs regs;

  if (::ptrace(PT_GETSPUREGS, pid, (caddr_t)&regs, 0) < 0)
    return Platform::TranslateError();

  // Copy all 128 general-purpose 128-bit registers
  for (size_t n = 0; n < 128; n++) {
    for (size_t i = 0; i < 4; i++) {
      state.cell_spu.gpr[n].word[i] = regs.gpr[n][i];
    }
  }

  // Copy special registers
  state.cell_spu.pc = regs.pc;
  state.cell_spu.npc = regs.npc;
  state.cell_spu.fpscr = regs.fpscr;
  state.cell_spu.decrementer = regs.decrementer;
  state.cell_spu.event_mask = regs.event_mask;
  state.cell_spu.event_status = regs.event_status;
  state.cell_spu.signal1 = regs.signal1;
  state.cell_spu.signal2 = regs.signal2;
  state.cell_spu.mb_stat = regs.mb_stat;
  state.cell_spu.mb_data = regs.mb_data;
  state.cell_spu.status = regs.status;
  state.cell_spu.lslr = regs.lslr;

  // Read MFC (Memory Flow Controller) command queue if available
  // FreeBSD may provide this via a separate ptrace request
  uint32_t mfc_cq[16];
  if (::ptrace(PT_GETSPUMFC, pid, (caddr_t)&mfc_cq, 0) == 0) {
    for (size_t n = 0; n < 16; n++) {
      state.cell_spu.mfc_cq[n] = mfc_cq[n];
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;

  // Prepare SPU register structure
  struct spu_regs regs;

  for (size_t n = 0; n < 128; n++) {
    for (size_t i = 0; i < 4; i++) {
      regs.gpr[n][i] = state.cell_spu.gpr[n].word[i];
    }
  }

  regs.pc = state.cell_spu.pc;
  regs.npc = state.cell_spu.npc;
  regs.fpscr = state.cell_spu.fpscr;
  regs.decrementer = state.cell_spu.decrementer;
  regs.event_mask = state.cell_spu.event_mask;
  regs.event_status = state.cell_spu.event_status;
  regs.signal1 = state.cell_spu.signal1;
  regs.signal2 = state.cell_spu.signal2;
  regs.mb_stat = state.cell_spu.mb_stat;
  regs.mb_data = state.cell_spu.mb_data;
  regs.status = state.cell_spu.status;
  regs.lslr = state.cell_spu.lslr;

  if (::ptrace(PT_SETSPUREGS, pid, (caddr_t)&regs, 0) < 0)
    return Platform::TranslateError();

  // Write MFC command queue if available
  uint32_t mfc_cq[16];
  for (size_t n = 0; n < 16; n++) {
    mfc_cq[n] = state.cell_spu.mfc_cq[n];
  }

  if (::ptrace(PT_SETSPUMFC, pid, (caddr_t)&mfc_cq, 0) < 0) {
    // Non-fatal
  }

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
