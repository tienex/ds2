//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <mach/mach.h>
#include <mach/ppc/thread_status.h>

namespace ds2 {
namespace Host {
namespace Darwin {

ErrorCode Mach::readCPUState(ProcessThreadId const &ptid,
                             ProcessInfo const &pinfo,
                             Architecture::CPUState &state) {
  mach_port_t task = getTask(ptid.pid);
  if (task == MACH_PORT_NULL)
    return kErrorProcessNotFound;

  thread_t thread = getThread(ptid);
  if (thread == MACH_PORT_NULL)
    return kErrorProcessNotFound;

  // Read PowerPC64 thread state
  ppc_thread_state64_t gprs;
  mach_msg_type_number_t count = PPC_THREAD_STATE64_COUNT;

  kern_return_t kr = thread_get_state(thread, PPC_THREAD_STATE64,
                                      (thread_state_t)&gprs, &count);
  if (kr != KERN_SUCCESS)
    return Platform::TranslateError(kr);

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    state.ppc64.gp.regs[n] = gprs.r[n];
  }

  // Copy special registers
  state.ppc64.pc = gprs.srr0;  // PC is in SRR0
  state.ppc64.msr = gprs.srr1; // MSR is in SRR1
  state.ppc64.lr = gprs.lr;
  state.ppc64.ctr = gprs.ctr;
  state.ppc64.xer = gprs.xer;
  state.ppc64.cr = gprs.cr;

  // Read floating-point state
  ppc_float_state_t fprs;
  count = PPC_FLOAT_STATE_COUNT;

  kr = thread_get_state(thread, PPC_FLOAT_STATE,
                        (thread_state_t)&fprs, &count);
  if (kr == KERN_SUCCESS) {
    for (size_t n = 0; n < 32; n++) {
      state.ppc64.fp.regs[n] = fprs.fpregs[n];
    }
    state.ppc64.fpscr = fprs.fpscr;
  }

  // Read AltiVec/VMX state
  ppc_vector_state_t vrs;
  count = PPC_VECTOR_STATE_COUNT;

  kr = thread_get_state(thread, PPC_VECTOR_STATE,
                        (thread_state_t)&vrs, &count);
  if (kr == KERN_SUCCESS) {
    for (size_t n = 0; n < 32; n++) {
      for (size_t i = 0; i < 4; i++) {
        state.ppc64.vr[n].v[i] = vrs.save_vr[n][i];
      }
    }
    state.ppc64.vscr = vrs.save_vscr[3]; // VSCR is in word 3
    state.ppc64.vrsave = vrs.save_vrsave;
  }

  return kSuccess;
}

ErrorCode Mach::writeCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState const &state) {
  mach_port_t task = getTask(ptid.pid);
  if (task == MACH_PORT_NULL)
    return kErrorProcessNotFound;

  thread_t thread = getThread(ptid);
  if (thread == MACH_PORT_NULL)
    return kErrorProcessNotFound;

  // Prepare PowerPC64 thread state
  ppc_thread_state64_t gprs;
  mach_msg_type_number_t count = PPC_THREAD_STATE64_COUNT;

  // Copy general-purpose registers
  for (size_t n = 0; n < 32; n++) {
    gprs.r[n] = state.ppc64.gp.regs[n];
  }

  // Copy special registers
  gprs.srr0 = state.ppc64.pc;
  gprs.srr1 = state.ppc64.msr;
  gprs.lr = state.ppc64.lr;
  gprs.ctr = state.ppc64.ctr;
  gprs.xer = state.ppc64.xer;
  gprs.cr = state.ppc64.cr;

  kern_return_t kr = thread_set_state(thread, PPC_THREAD_STATE64,
                                      (thread_state_t)&gprs, count);
  if (kr != KERN_SUCCESS)
    return Platform::TranslateError(kr);

  // Write floating-point state
  ppc_float_state_t fprs;
  count = PPC_FLOAT_STATE_COUNT;

  for (size_t n = 0; n < 32; n++) {
    fprs.fpregs[n] = state.ppc64.fp.regs[n];
  }
  fprs.fpscr = state.ppc64.fpscr;

  kr = thread_set_state(thread, PPC_FLOAT_STATE,
                        (thread_state_t)&fprs, count);
  if (kr != KERN_SUCCESS) {
    // Non-fatal if FPU not available
  }

  // Write AltiVec/VMX state
  ppc_vector_state_t vrs;
  count = PPC_VECTOR_STATE_COUNT;

  for (size_t n = 0; n < 32; n++) {
    for (size_t i = 0; i < 4; i++) {
      vrs.save_vr[n][i] = state.ppc64.vr[n].v[i];
    }
  }
  vrs.save_vscr[3] = state.ppc64.vscr; // VSCR is in word 3
  vrs.save_vrsave = state.ppc64.vrsave;

  kr = thread_set_state(thread, PPC_VECTOR_STATE,
                        (thread_state_t)&vrs, count);
  if (kr != KERN_SUCCESS) {
    // Non-fatal if AltiVec not available
  }

  return kSuccess;
}

} // namespace Darwin
} // namespace Host
} // namespace ds2
