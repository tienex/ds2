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
#include "DebugServer2/Architecture/IA64/CPUState.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Read general registers using PTRACE_GETREGS
  // Linux IA-64 pt_regs structure
  //
  struct pt_regs gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  //
  // Copy general-purpose registers (GR0-GR127)
  // Linux exposes a subset via pt_regs
  //
  for (int i = 0; i < 128; i++) {
    if (i < 32) {
      // pt_regs provides r1-r31 (r0 is hardwired to zero)
      state.gp.regs[i] = (i == 0) ? 0 : gprs.r1 + (i - 1) * sizeof(unsigned long);
    } else {
      // Stacked registers accessed via backing store
      state.gp.regs[i] = 0; // Will be read from backing store
    }
  }

  //
  // Copy branch registers (b0-b7)
  //
  state.br[0] = gprs.b0; // Return pointer
  for (int i = 1; i < 8; i++) {
    state.br[i] = gprs.b6 + (i - 1) * sizeof(unsigned long);
  }

  //
  // Copy predicate registers
  //
  state.pr = gprs.pr;

  //
  // Copy instruction pointer
  //
  state.ip = gprs.cr_iip;

  //
  // Copy application registers
  //
  state.ar.rsc = gprs.ar_rsc;
  state.ar.bsp = gprs.ar_bsp;
  state.ar.bspstore = gprs.ar_bspstore;
  state.ar.rnat = gprs.ar_rnat;
  state.ar.ccv = gprs.ar_ccv;
  state.ar.unat = gprs.ar_unat;
  state.ar.fpsr = gprs.ar_fpsr;
  state.ar.pfs = gprs.ar_pfs;
  state.ar.lc = gprs.ar_lc;
  state.ar.ec = gprs.ar_ec;

  //
  // Copy control registers
  //
  state.cr.ipsr = gprs.cr_ipsr;
  state.cr.isr = gprs.cr_isr;
  state.cr.iip = gprs.cr_iip;
  state.cr.ifa = gprs.cr_ifa;
  state.cr.itir = gprs.cr_itir;
  state.cr.iipa = gprs.cr_iipa;
  state.cr.ifs = gprs.cr_ifs;
  state.cr.iim = gprs.cr_iim;
  state.cr.iha = gprs.cr_iha;

  //
  // Copy processor status register
  //
  std::memcpy(&state.psr, &gprs.cr_ipsr, sizeof(state.psr));

  //
  // Copy current frame marker
  //
  std::memcpy(&state.cfm, &gprs.cfm, sizeof(state.cfm));

  //
  // Copy NaT bits
  //
  state.nat.nat_low = gprs.nat & 0xFFFFFFFFFFFFFFFFULL;
  state.nat.nat_high = (gprs.nat >> 64) & 0xFFFFFFFFFFFFFFFFULL;

#if defined(PTRACE_GETFPREGS)
  //
  // Read floating-point registers
  //
  elf_fpregset_t fpregs;
  if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fpregs) >= 0) {
    //
    // Copy floating-point registers (FR0-FR127)
    // FR0 and FR1 are hardwired
    //
    state.fpr[0].significand = 0;
    state.fpr[0].exponent = 0;
    state.fpr[0].sign = 0; // +0.0

    state.fpr[1].significand = 1ULL << 63;
    state.fpr[1].exponent = 0xFFFF;
    state.fpr[1].sign = 0; // +1.0

    // Copy user-accessible FP registers (FR2-FR127)
    for (int i = 2; i < 128; i++) {
      state.fpr[i].significand = fpregs.fr[i].significand;
      state.fpr[i].exponent = fpregs.fr[i].exponent & 0x1FFFF;
      state.fpr[i].sign = (fpregs.fr[i].exponent >> 17) & 1;
    }
  }
#endif

#if defined(PTRACE_GETREGSET)
  //
  // Read extended registers using PTRACE_GETREGSET
  //
  struct iovec iov;

  // Read debug registers (NT_IA64_DEBUG)
  struct {
    unsigned long ibr[8][2];
    unsigned long dbr[8][2];
  } debug_regs;
  iov.iov_base = &debug_regs;
  iov.iov_len = sizeof(debug_regs);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_IA64_DEBUG, &iov) >= 0) {
    for (int i = 0; i < 8; i++) {
      state.debug.ibr[i].addr = debug_regs.ibr[i][0];
      state.debug.ibr[i].mask = debug_regs.ibr[i][1];
      state.debug.dbr[i].addr = debug_regs.dbr[i][0];
      state.debug.dbr[i].mask = debug_regs.dbr[i][1];
    }
  }

  // Read region registers (NT_IA64_RR)
  unsigned long rr[8];
  iov.iov_base = rr;
  iov.iov_len = sizeof(rr);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_IA64_RR, &iov) >= 0) {
    for (int i = 0; i < 8; i++) {
      std::memcpy(&state.rr[i], &rr[i], sizeof(state.rr[i]));
    }
  }

  // Read protection key registers (NT_IA64_PKR)
  unsigned long pkr[16];
  iov.iov_base = pkr;
  iov.iov_len = sizeof(pkr);

  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_IA64_PKR, &iov) >= 0) {
    for (int i = 0; i < 16; i++) {
      std::memcpy(&state.pkr[i], &pkr[i], sizeof(state.pkr[i]));
    }
  }
#endif

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Prepare general register set
  //
  struct pt_regs gprs;
  std::memset(&gprs, 0, sizeof(gprs));

  //
  // Copy general-purpose registers (GR1-GR31)
  //
  for (int i = 1; i < 32; i++) {
    *(&gprs.r1 + (i - 1)) = state.gp.regs[i];
  }

  //
  // Copy branch registers
  //
  gprs.b0 = state.br[0];
  for (int i = 1; i < 8; i++) {
    *(&gprs.b6 + (i - 1)) = state.br[i];
  }

  //
  // Copy predicate registers
  //
  gprs.pr = state.pr;

  //
  // Copy instruction pointer
  //
  gprs.cr_iip = state.ip;

  //
  // Copy application registers
  //
  gprs.ar_rsc = state.ar.rsc;
  gprs.ar_bsp = state.ar.bsp;
  gprs.ar_bspstore = state.ar.bspstore;
  gprs.ar_rnat = state.ar.rnat;
  gprs.ar_ccv = state.ar.ccv;
  gprs.ar_unat = state.ar.unat;
  gprs.ar_fpsr = state.ar.fpsr;
  gprs.ar_pfs = state.ar.pfs;
  gprs.ar_lc = state.ar.lc;
  gprs.ar_ec = state.ar.ec;

  //
  // Copy control registers
  //
  gprs.cr_ipsr = state.cr.ipsr;
  gprs.cr_isr = state.cr.isr;
  gprs.cr_iip = state.cr.iip;
  gprs.cr_ifa = state.cr.ifa;
  gprs.cr_itir = state.cr.itir;
  gprs.cr_iipa = state.cr.iipa;
  gprs.cr_ifs = state.cr.ifs;
  gprs.cr_iim = state.cr.iim;
  gprs.cr_iha = state.cr.iha;

  //
  // Copy current frame marker
  //
  std::memcpy(&gprs.cfm, &state.cfm, sizeof(gprs.cfm));

  //
  // Copy NaT bits
  //
  gprs.nat = state.nat.nat_low | (static_cast<__uint128_t>(state.nat.nat_high) << 64);

  //
  // Write general registers
  //
  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

#if defined(PTRACE_SETFPREGS)
  //
  // Write floating-point registers
  //
  elf_fpregset_t fpregs;
  std::memset(&fpregs, 0, sizeof(fpregs));

  for (int i = 2; i < 128; i++) {
    fpregs.fr[i].significand = state.fpr[i].significand;
    fpregs.fr[i].exponent = (state.fpr[i].exponent & 0x1FFFF) |
                            ((state.fpr[i].sign & 1) << 17);
  }

  if (wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fpregs) < 0)
    return Platform::TranslateError();
#endif

#if defined(PTRACE_SETREGSET)
  //
  // Write extended registers using PTRACE_SETREGSET
  //
  struct iovec iov;

  // Write debug registers
  struct {
    unsigned long ibr[8][2];
    unsigned long dbr[8][2];
  } debug_regs;

  for (int i = 0; i < 8; i++) {
    debug_regs.ibr[i][0] = state.debug.ibr[i].addr;
    debug_regs.ibr[i][1] = state.debug.ibr[i].mask;
    debug_regs.dbr[i][0] = state.debug.dbr[i].addr;
    debug_regs.dbr[i][1] = state.debug.dbr[i].mask;
  }

  iov.iov_base = &debug_regs;
  iov.iov_len = sizeof(debug_regs);
  wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_IA64_DEBUG, &iov);

  // Write region registers
  unsigned long rr[8];
  for (int i = 0; i < 8; i++) {
    std::memcpy(&rr[i], &state.rr[i], sizeof(rr[i]));
  }

  iov.iov_base = rr;
  iov.iov_len = sizeof(rr);
  wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_IA64_RR, &iov);

  // Write protection key registers
  unsigned long pkr[16];
  for (int i = 0; i < 16; i++) {
    std::memcpy(&pkr[i], &state.pkr[i], sizeof(pkr[i]));
  }

  iov.iov_base = pkr;
  iov.iov_len = sizeof(pkr);
  wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_IA64_PKR, &iov);
#endif

  return kSuccess;
}

} // namespace Linux
} // namespace Host
} // namespace ds2
