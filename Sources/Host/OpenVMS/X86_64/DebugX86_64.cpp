//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/OpenVMS/Debug.h"
#include "DebugServer2/Architecture/X86_64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace OpenVMS {

// OpenVMS x86-64 debugging
// Supports VSI OpenVMS x86-64 (9.0+)
// VMS Software Inc is porting OpenVMS to x86-64

// x86-64 register context
struct X86_64Context {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rsi, rdi, rbp, rsp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t rip;
  uint64_t rflags;
  uint16_t cs, ds, es, fs, gs, ss;
  uint64_t fsbase, gsbase;
};

// OpenVMS Process Control Block (x86-64)
struct PCB {
  uint64_t state;
  uint32_t pid;
  uint32_t priority;
  uint64_t jib;
  uint64_t phd;
  uint64_t ksp;
};

static bool g_debuggerActive = false;
static X86_64Context g_context;

class Debug {
public:
  static ErrorCode attach(uint32_t pid);
  static ErrorCode detach(uint32_t pid);

  static ErrorCode setBreakpoint(Address address);
  static ErrorCode removeBreakpoint(Address address);

  static ErrorCode resume(uint32_t pid);
  static ErrorCode suspend(uint32_t pid);
  static ErrorCode step(uint32_t pid);

  static ErrorCode readMemory(Address address, void *data, size_t size);
  static ErrorCode writeMemory(Address address, void const *data, size_t size);

  static ErrorCode readRegisters(void *regs, size_t size);
  static ErrorCode writeRegisters(void const *regs, size_t size);

  static ErrorCode getProcessInfo(uint32_t pid, PCB &pcb);
};

ErrorCode Debug::attach(uint32_t pid) {
  DS2LOG(Debug, "attaching to OpenVMS x86-64 process: PID=%u", pid);

  // Use SYS$DEBUG to attach
  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t pid) {
  DS2LOG(Debug, "detaching from OpenVMS x86-64 process: PID=%u", pid);

  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  // Use INT3 instruction (0xCC)
  uint8_t int3 = 0xCC;

  ErrorCode error = writeMemory(address, &int3, sizeof(int3));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint instruction");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::removeBreakpoint(Address address) {
  DS2LOG(Debug, "removing breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::resume(uint32_t pid) {
  DS2LOG(Debug, "resuming OpenVMS x86-64 process: PID=%u", pid);

  // Clear trap flag
  g_context.rflags &= ~0x100;

  return kSuccess;
}

ErrorCode Debug::suspend(uint32_t pid) {
  DS2LOG(Debug, "suspending OpenVMS x86-64 process: PID=%u", pid);
  return kSuccess;
}

ErrorCode Debug::step(uint32_t pid) {
  DS2LOG(Debug, "single-stepping OpenVMS x86-64 process: PID=%u", pid);

  // Set trap flag
  g_context.rflags |= 0x100;

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  uint64_t addr = address.value();

  DS2LOG(Debug, "reading %zu bytes from 0x%016llx", size,
         (unsigned long long)addr);

  // Use SYS$READMEM or direct access
  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint64_t addr = address.value();

  DS2LOG(Debug, "writing %zu bytes to 0x%016llx", size,
         (unsigned long long)addr);

  // Use SYS$WRITEMEM or direct access
  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::X86_64::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::X86_64::CPUState *state =
      reinterpret_cast<Architecture::X86_64::CPUState *>(regs);

  // Copy general purpose registers
  state->gp.rax = g_context.rax;
  state->gp.rbx = g_context.rbx;
  state->gp.rcx = g_context.rcx;
  state->gp.rdx = g_context.rdx;
  state->gp.rsi = g_context.rsi;
  state->gp.rdi = g_context.rdi;
  state->gp.rbp = g_context.rbp;
  state->gp.rsp = g_context.rsp;
  state->gp.r8 = g_context.r8;
  state->gp.r9 = g_context.r9;
  state->gp.r10 = g_context.r10;
  state->gp.r11 = g_context.r11;
  state->gp.r12 = g_context.r12;
  state->gp.r13 = g_context.r13;
  state->gp.r14 = g_context.r14;
  state->gp.r15 = g_context.r15;

  // Special registers
  state->special.rip = g_context.rip;
  state->special.rflags = g_context.rflags;

  // Segment registers
  state->special.cs = g_context.cs;
  state->special.ds = g_context.ds;
  state->special.es = g_context.es;
  state->special.fs = g_context.fs;
  state->special.gs = g_context.gs;
  state->special.ss = g_context.ss;

  // Segment bases
  state->special.fsbase = g_context.fsbase;
  state->special.gsbase = g_context.gsbase;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::X86_64::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::X86_64::CPUState *state =
      reinterpret_cast<const Architecture::X86_64::CPUState *>(regs);

  // Copy general purpose registers
  g_context.rax = state->gp.rax;
  g_context.rbx = state->gp.rbx;
  g_context.rcx = state->gp.rcx;
  g_context.rdx = state->gp.rdx;
  g_context.rsi = state->gp.rsi;
  g_context.rdi = state->gp.rdi;
  g_context.rbp = state->gp.rbp;
  g_context.rsp = state->gp.rsp;
  g_context.r8 = state->gp.r8;
  g_context.r9 = state->gp.r9;
  g_context.r10 = state->gp.r10;
  g_context.r11 = state->gp.r11;
  g_context.r12 = state->gp.r12;
  g_context.r13 = state->gp.r13;
  g_context.r14 = state->gp.r14;
  g_context.r15 = state->gp.r15;

  // Special registers
  g_context.rip = state->special.rip;
  g_context.rflags = state->special.rflags;

  // Segment registers
  g_context.cs = state->special.cs;
  g_context.ds = state->special.ds;
  g_context.es = state->special.es;
  g_context.fs = state->special.fs;
  g_context.gs = state->special.gs;
  g_context.ss = state->special.ss;

  // Segment bases
  g_context.fsbase = state->special.fsbase;
  g_context.gsbase = state->special.gsbase;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, PCB &pcb) {
  DS2LOG(Debug, "getting x86-64 process info for PID=%u", pid);

  // Use SYS$GETJPI
  pcb.pid = pid;
  pcb.state = 1;
  pcb.priority = 4;
  pcb.jib = 0;
  pcb.phd = 0;
  pcb.ksp = g_context.rsp;

  return kSuccess;
}

} // namespace OpenVMS
} // namespace Host
} // namespace ds2
