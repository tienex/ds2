//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PlayStation 3 Cell Broadband Engine - PPU (PowerPC Processing Element)
// FreeBSD port - used in PS3 homebrew and development
//

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid = ptid.pid;

  // Read general-purpose and special registers
  struct reg gprs;
  if (::ptrace(PT_GETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // FreeBSD Cell PPU register structure layout (PowerPC64)
  // Copy GPRs
  for (size_t n = 0; n < 32; n++) {
    state.cell_ppu.gp.regs[n] = gprs.fixreg[n];
  }

  // Copy special registers
  state.cell_ppu.pc = gprs.pc;
  state.cell_ppu.msr = gprs.msr;
  state.cell_ppu.lr = gprs.lr;
  state.cell_ppu.ctr = gprs.ctr;
  state.cell_ppu.cr = gprs.cr;
  state.cell_ppu.xer = gprs.xer;

  // Read floating-point registers
  struct fpreg fprs;
  if (::ptrace(PT_GETFPREGS, pid, (caddr_t)&fprs, 0) == 0) {
    for (size_t n = 0; n < 32; n++) {
      state.cell_ppu.fp.regs[n] = fprs.fpreg[n];
    }
    state.cell_ppu.fpscr = fprs.fpscr;
  }

  // AltiVec/VMX registers
  // FreeBSD may provide these via PT_GETVECREGS on Cell-capable systems
  struct {
    uint32_t vr[32][4];
    uint32_t vscr;
    uint32_t vrsave;
  } vrregs;

  if (::ptrace(PT_GETVECREGS, pid, (caddr_t)&vrregs, 0) == 0) {
    for (size_t n = 0; n < 32; n++) {
      for (size_t i = 0; i < 4; i++) {
        state.cell_ppu.vr[n].v[i] = vrregs.vr[n][i];
      }
    }
    state.cell_ppu.vscr = vrregs.vscr;
    state.cell_ppu.vrsave = vrregs.vrsave;
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid = ptid.pid;

  // Prepare general-purpose and special registers
  struct reg gprs;

  for (size_t n = 0; n < 32; n++) {
    gprs.fixreg[n] = state.cell_ppu.gp.regs[n];
  }

  gprs.pc = state.cell_ppu.pc;
  gprs.msr = state.cell_ppu.msr;
  gprs.lr = state.cell_ppu.lr;
  gprs.ctr = state.cell_ppu.ctr;
  gprs.cr = state.cell_ppu.cr;
  gprs.xer = state.cell_ppu.xer;

  if (::ptrace(PT_SETREGS, pid, (caddr_t)&gprs, 0) < 0)
    return Platform::TranslateError();

  // Write floating-point registers
  struct fpreg fprs;

  for (size_t n = 0; n < 32; n++) {
    fprs.fpreg[n] = state.cell_ppu.fp.regs[n];
  }
  fprs.fpscr = state.cell_ppu.fpscr;

  if (::ptrace(PT_SETFPREGS, pid, (caddr_t)&fprs, 0) < 0) {
    // Non-fatal if FPU not available
  }

  // Write AltiVec/VMX registers
  struct {
    uint32_t vr[32][4];
    uint32_t vscr;
    uint32_t vrsave;
  } vrregs;

  for (size_t n = 0; n < 32; n++) {
    for (size_t i = 0; i < 4; i++) {
      vrregs.vr[n][i] = state.cell_ppu.vr[n].v[i];
    }
  }
  vrregs.vscr = state.cell_ppu.vscr;
  vrregs.vrsave = state.cell_ppu.vrsave;

  if (::ptrace(PT_SETVECREGS, pid, (caddr_t)&vrregs, 0) < 0) {
    // Non-fatal if AltiVec not available
  }

  return kSuccess;
}

} // namespace FreeBSD
} // namespace Host
} // namespace ds2
