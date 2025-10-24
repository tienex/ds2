//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/NetBSD/PTrace.h"
#include "DebugServer2/Architecture/IA64/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/ptrace.h>
#include <machine/reg.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace NetBSD {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  //
  // Read general registers using PT_GETREGS
  // NetBSD IA-64 struct reg
  //
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

  //
  // Copy general-purpose registers (r1-r31, r0 is hardwired to zero)
  //
  state.gp.regs[0] = 0;
  for (int i = 1; i < 128; i++) {
    if (i < 32) {
      state.gp.regs[i] = gprs.r_gr[i];
    } else {
      // Stacked registers from backing store
      state.gp.regs[i] = 0;
    }
  }

  //
  // Copy branch registers
  //
  for (int i = 0; i < 8; i++) {
    state.br[i] = gprs.r_br[i];
  }

  //
  // Copy predicate registers
  //
  state.pr = gprs.r_pr;

  //
  // Copy instruction pointer
  //
  state.ip = gprs.r_ip;

  //
  // Copy application registers
  //
  state.ar.rsc = gprs.r_ar_rsc;
  state.ar.bsp = gprs.r_ar_bsp;
  state.ar.bspstore = gprs.r_ar_bspstore;
  state.ar.rnat = gprs.r_ar_rnat;
  state.ar.ccv = gprs.r_ar_ccv;
  state.ar.unat = gprs.r_ar_unat;
  state.ar.fpsr = gprs.r_ar_fpsr;
  state.ar.pfs = gprs.r_ar_pfs;
  state.ar.lc = gprs.r_ar_lc;
  state.ar.ec = gprs.r_ar_ec;

  //
  // Copy control registers
  //
  state.cr.ipsr = gprs.r_cr_ipsr;
  state.cr.isr = gprs.r_cr_isr;
  state.cr.iip = gprs.r_cr_iip;
  state.cr.ifa = gprs.r_cr_ifa;
  state.cr.itir = gprs.r_cr_itir;
  state.cr.iipa = gprs.r_cr_iipa;
  state.cr.ifs = gprs.r_cr_ifs;
  state.cr.iim = gprs.r_cr_iim;
  state.cr.iha = gprs.r_cr_iha;

  //
  // Copy processor status register
  //
  std::memcpy(&state.psr, &gprs.r_cr_ipsr, sizeof(state.psr));

  //
  // Copy current frame marker
  //
  std::memcpy(&state.cfm, &gprs.r_cfm, sizeof(state.cfm));

  //
  // Copy NaT bits
  //
  state.nat.nat_low = gprs.r_nat & 0xFFFFFFFFFFFFFFFFULL;
  state.nat.nat_high = (gprs.r_nat >> 64) & 0xFFFFFFFFFFFFFFFFULL;

#if defined(PT_GETFPREGS)
  //
  // Read floating-point registers
  //
  struct fpreg fpregs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fpregs, 0) >= 0) {
    //
    // Copy floating-point registers (FR0-FR127)
    // FR0 and FR1 are hardwired
    //
    state.fpr[0].significand = 0;
    state.fpr[0].exponent = 0;
    state.fpr[0].sign = 0;

    state.fpr[1].significand = 1ULL << 63;
    state.fpr[1].exponent = 0xFFFF;
    state.fpr[1].sign = 0;

    // Copy user FP registers
    for (int i = 2; i < 128; i++) {
      state.fpr[i].significand = fpregs.fpr_fr[i].significand;
      state.fpr[i].exponent = fpregs.fpr_fr[i].exponent & 0x1FFFF;
      state.fpr[i].sign = (fpregs.fpr_fr[i].exponent >> 17) & 1;
    }
  }
#endif

#if defined(PT_GETDBREGS)
  //
  // Read debug registers
  //
  struct dbreg dbregs;
  if (wrapPtrace(PT_GETDBREGS, pid, &dbregs, 0) >= 0) {
    for (int i = 0; i < 8; i++) {
      state.debug.ibr[i].addr = dbregs.dbr_ibr[i][0];
      state.debug.ibr[i].mask = dbregs.dbr_ibr[i][1];
      state.debug.dbr[i].addr = dbregs.dbr_dbr[i][0];
      state.debug.dbr[i].mask = dbregs.dbr_dbr[i][1];
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
  struct reg gprs;
  std::memset(&gprs, 0, sizeof(gprs));

  //
  // Copy general-purpose registers
  //
  for (int i = 1; i < 32; i++) {
    gprs.r_gr[i] = state.gp.regs[i];
  }

  //
  // Copy branch registers
  //
  for (int i = 0; i < 8; i++) {
    gprs.r_br[i] = state.br[i];
  }

  //
  // Copy predicate registers
  //
  gprs.r_pr = state.pr;

  //
  // Copy instruction pointer
  //
  gprs.r_ip = state.ip;

  //
  // Copy application registers
  //
  gprs.r_ar_rsc = state.ar.rsc;
  gprs.r_ar_bsp = state.ar.bsp;
  gprs.r_ar_bspstore = state.ar.bspstore;
  gprs.r_ar_rnat = state.ar.rnat;
  gprs.r_ar_ccv = state.ar.ccv;
  gprs.r_ar_unat = state.ar.unat;
  gprs.r_ar_fpsr = state.ar.fpsr;
  gprs.r_ar_pfs = state.ar.pfs;
  gprs.r_ar_lc = state.ar.lc;
  gprs.r_ar_ec = state.ar.ec;

  //
  // Copy control registers
  //
  gprs.r_cr_ipsr = state.cr.ipsr;
  gprs.r_cr_isr = state.cr.isr;
  gprs.r_cr_iip = state.cr.iip;
  gprs.r_cr_ifa = state.cr.ifa;
  gprs.r_cr_itir = state.cr.itir;
  gprs.r_cr_iipa = state.cr.iipa;
  gprs.r_cr_ifs = state.cr.ifs;
  gprs.r_cr_iim = state.cr.iim;
  gprs.r_cr_iha = state.cr.iha;

  //
  // Copy current frame marker
  //
  std::memcpy(&gprs.r_cfm, &state.cfm, sizeof(gprs.r_cfm));

  //
  // Copy NaT bits
  //
  gprs.r_nat = state.nat.nat_low | (static_cast<__uint128_t>(state.nat.nat_high) << 64);

  //
  // Write general registers
  //
  if (wrapPtrace(PT_SETREGS, pid, &gprs, 0) < 0)
    return Platform::TranslateError();

#if defined(PT_SETFPREGS)
  //
  // Write floating-point registers
  //
  struct fpreg fpregs;
  std::memset(&fpregs, 0, sizeof(fpregs));

  for (int i = 2; i < 128; i++) {
    fpregs.fpr_fr[i].significand = state.fpr[i].significand;
    fpregs.fpr_fr[i].exponent = (state.fpr[i].exponent & 0x1FFFF) |
                                ((state.fpr[i].sign & 1) << 17);
  }

  if (wrapPtrace(PT_SETFPREGS, pid, &fpregs, 0) < 0)
    return Platform::TranslateError();
#endif

#if defined(PT_SETDBREGS)
  //
  // Write debug registers
  //
  struct dbreg dbregs;
  std::memset(&dbregs, 0, sizeof(dbregs));

  for (int i = 0; i < 8; i++) {
    dbregs.dbr_ibr[i][0] = state.debug.ibr[i].addr;
    dbregs.dbr_ibr[i][1] = state.debug.ibr[i].mask;
    dbregs.dbr_dbr[i][0] = state.debug.dbr[i].addr;
    dbregs.dbr_dbr[i][1] = state.debug.dbr[i].mask;
  }

  if (wrapPtrace(PT_SETDBREGS, pid, &dbregs, 0) < 0)
    return Platform::TranslateError();
#endif

  return kSuccess;
}

} // namespace NetBSD
} // namespace Host
} // namespace ds2
