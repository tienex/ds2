//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/HPUX/ProcFS.h"
#include "DebugServer2/Architecture/IA64_32/CPUState.h"
#include "DebugServer2/Host/Platform.h"

#include <sys/procfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace ds2 {
namespace Host {
namespace HPUX {

//
// HP-UX IA-64 32-bit mode (ILP32) procfs-based debugging
// Uses /proc/<pid>/reg for register access
// Register layout is same as IA-64 but in 32-bit ABI mode
//

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", ptid.pid, ptid.tid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // HP-UX ILP32 mode uses same prgregset_t but for 32-bit ABI
  //
  prgregset_t gregs;
  if (::pread(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Copy general-purpose registers (GR0-GR127)
  // In ILP32 mode, upper 32 bits of pointers are masked but we read full state
  //
  for (int i = 0; i < 128; i++) {
    state.gp.regs[i] = gregs.gr[i];
  }

  //
  // Copy branch registers
  //
  for (int i = 0; i < 8; i++) {
    state.br[i] = gregs.br[i];
  }

  //
  // Copy predicate registers
  //
  state.pr = gregs.pr;

  //
  // Copy instruction pointer
  //
  state.ip = gregs.ip;

  //
  // Copy application registers
  //
  state.ar.rsc = gregs.ar_rsc;
  state.ar.bsp = gregs.ar_bsp;
  state.ar.bspstore = gregs.ar_bspstore;
  state.ar.rnat = gregs.ar_rnat;
  state.ar.ccv = gregs.ar_ccv;
  state.ar.unat = gregs.ar_unat;
  state.ar.fpsr = gregs.ar_fpsr;
  state.ar.itc = gregs.ar_itc;
  state.ar.pfs = gregs.ar_pfs;
  state.ar.lc = gregs.ar_lc;
  state.ar.ec = gregs.ar_ec;

  //
  // Copy control registers
  //
  state.cr.dcr = gregs.cr_dcr;
  state.cr.itm = gregs.cr_itm;
  state.cr.iva = gregs.cr_iva;
  state.cr.pta = gregs.cr_pta;
  state.cr.ipsr = gregs.cr_ipsr;
  state.cr.isr = gregs.cr_isr;
  state.cr.iip = gregs.cr_iip;
  state.cr.ifa = gregs.cr_ifa;
  state.cr.itir = gregs.cr_itir;
  state.cr.iipa = gregs.cr_iipa;
  state.cr.ifs = gregs.cr_ifs;
  state.cr.iim = gregs.cr_iim;
  state.cr.iha = gregs.cr_iha;

  //
  // Copy processor status register
  //
  std::memcpy(&state.psr, &gregs.cr_ipsr, sizeof(state.psr));

  //
  // Copy current frame marker
  //
  std::memcpy(&state.cfm, &gregs.cfm, sizeof(state.cfm));

  //
  // Copy NaT bits
  //
  state.nat.nat_low = gregs.nat & 0xFFFFFFFFFFFFFFFFULL;
  state.nat.nat_high = (gregs.nat >> 64) & 0xFFFFFFFFFFFFFFFFULL;

  //
  // Read floating-point registers
  //
  prfpregset_t fpregs;
  if (::pread(fd, &fpregs, sizeof(fpregs), sizeof(gregs)) == sizeof(fpregs)) {
    //
    // Copy floating-point registers (FR0-FR127)
    //
    for (int i = 0; i < 128; i++) {
      state.fpr[i].significand = fpregs.fr[i].significand;
      state.fpr[i].exponent = fpregs.fr[i].exponent & 0x1FFFF;
      state.fpr[i].sign = (fpregs.fr[i].exponent >> 17) & 1;
    }
  }

  ::close(fd);
  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%d/lwpctl", ptid.pid, ptid.tid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  //
  // Prepare general register set
  //
  prgregset_t gregs;
  std::memset(&gregs, 0, sizeof(gregs));

  //
  // Copy general-purpose registers
  //
  for (int i = 0; i < 128; i++) {
    gregs.gr[i] = state.gp.regs[i];
  }

  //
  // Copy branch registers
  //
  for (int i = 0; i < 8; i++) {
    gregs.br[i] = state.br[i];
  }

  //
  // Copy predicate registers
  //
  gregs.pr = state.pr;

  //
  // Copy instruction pointer
  //
  gregs.ip = state.ip;

  //
  // Copy application registers
  //
  gregs.ar_rsc = state.ar.rsc;
  gregs.ar_bsp = state.ar.bsp;
  gregs.ar_bspstore = state.ar.bspstore;
  gregs.ar_rnat = state.ar.rnat;
  gregs.ar_ccv = state.ar.ccv;
  gregs.ar_unat = state.ar.unat;
  gregs.ar_fpsr = state.ar.fpsr;
  gregs.ar_itc = state.ar.itc;
  gregs.ar_pfs = state.ar.pfs;
  gregs.ar_lc = state.ar.lc;
  gregs.ar_ec = state.ar.ec;

  //
  // Copy control registers
  //
  gregs.cr_dcr = state.cr.dcr;
  gregs.cr_itm = state.cr.itm;
  gregs.cr_iva = state.cr.iva;
  gregs.cr_pta = state.cr.pta;
  gregs.cr_ipsr = state.cr.ipsr;
  gregs.cr_isr = state.cr.isr;
  gregs.cr_iip = state.cr.iip;
  gregs.cr_ifa = state.cr.ifa;
  gregs.cr_itir = state.cr.itir;
  gregs.cr_iipa = state.cr.iipa;
  gregs.cr_ifs = state.cr.ifs;
  gregs.cr_iim = state.cr.iim;
  gregs.cr_iha = state.cr.iha;

  //
  // Copy current frame marker
  //
  std::memcpy(&gregs.cfm, &state.cfm, sizeof(gregs.cfm));

  //
  // Copy NaT bits
  //
  gregs.nat = state.nat.nat_low | (static_cast<__uint128_t>(state.nat.nat_high) << 64);

  //
  // Write general registers
  //
  if (::pwrite(fd, &gregs, sizeof(gregs), 0) != sizeof(gregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  //
  // Write floating-point registers
  //
  prfpregset_t fpregs;
  std::memset(&fpregs, 0, sizeof(fpregs));

  for (int i = 0; i < 128; i++) {
    fpregs.fr[i].significand = state.fpr[i].significand;
    fpregs.fr[i].exponent = (state.fpr[i].exponent & 0x1FFFF) |
                            ((state.fpr[i].sign & 1) << 17);
  }

  if (::pwrite(fd, &fpregs, sizeof(fpregs), sizeof(gregs)) != sizeof(fpregs)) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ::close(fd);
  return kSuccess;
}

} // namespace HPUX
} // namespace Host
} // namespace ds2
