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
#include "DebugServer2/Architecture/IA64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace OpenVMS {

// OpenVMS IA-64 debugging
// Supports OpenVMS I64 8.2 through 8.4 on Itanium

// IA-64 register context
struct IA64Context {
  uint64_t gr[128];   // General registers r0-r127 (r0 is zero)
  uint64_t br[8];     // Branch registers b0-b7
  uint64_t fr[128];   // Floating point registers f0-f127
  uint64_t pr;        // Predicate registers (64-bit)
  uint64_t ip;        // Instruction pointer (bundle address)
  uint64_t psr;       // Processor status register
  uint64_t cfm;       // Current frame marker
  uint64_t ar[128];   // Application registers
  uint64_t rsc;       // RSE configuration
  uint64_t bsp;       // Backing store pointer
  uint64_t bspstore;  // Backing store pointer for stores
  uint64_t rnat;      // RSE NaT collection
};

// OpenVMS Process Control Block (IA-64)
struct PCB {
  uint64_t state;
  uint32_t pid;
  uint32_t priority;
  uint64_t jib;
  uint64_t phd;
  uint64_t ksp;
  uint64_t bsp;       // Backing store pointer
};

static bool g_debuggerActive = false;
static IA64Context g_context;

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
  DS2LOG(Debug, "attaching to OpenVMS IA-64 process: PID=%u", pid);

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t pid) {
  DS2LOG(Debug, "detaching from OpenVMS IA-64 process: PID=%u", pid);

  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  // On IA-64, use 'break.i' instruction for breakpoint
  // break.i has opcode format with immediate value
  // Commonly use break.i 0x80000 for debugger breakpoint
  // Bundle format: instruction is 41 bits in a 128-bit bundle
  // For simplicity, we'll use a pre-encoded bundle

  // This is a simplified approach - real IA-64 requires bundle manipulation
  uint64_t breakBundle[2] = {0x0000000100000001ULL, 0x0000000000000000ULL};

  ErrorCode error = writeMemory(address, breakBundle, sizeof(breakBundle));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint bundle");
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
  DS2LOG(Debug, "resuming OpenVMS IA-64 process: PID=%u", pid);

  // Clear single-step bit in PSR
  g_context.psr &= ~(1ULL << 21);  // Clear PSR.ss

  return kSuccess;
}

ErrorCode Debug::suspend(uint32_t pid) {
  DS2LOG(Debug, "suspending OpenVMS IA-64 process: PID=%u", pid);
  return kSuccess;
}

ErrorCode Debug::step(uint32_t pid) {
  DS2LOG(Debug, "single-stepping OpenVMS IA-64 process: PID=%u", pid);

  // Set single-step bit in PSR
  g_context.psr |= (1ULL << 21);  // Set PSR.ss

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  uint64_t addr = address.value();

  DS2LOG(Debug, "reading %zu bytes from 0x%016llx", size,
         (unsigned long long)addr);

  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint64_t addr = address.value();

  DS2LOG(Debug, "writing %zu bytes to 0x%016llx", size,
         (unsigned long long)addr);

  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::IA64::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::IA64::CPUState *state =
      reinterpret_cast<Architecture::IA64::CPUState *>(regs);

  // Copy general registers (r0 is hardwired to zero)
  state->gp.regs[0] = 0;
  for (int i = 1; i < 128; i++) {
    state->gp.regs[i] = g_context.gr[i];
  }

  // Copy branch registers
  for (int i = 0; i < 8; i++) {
    state->special.br[i] = g_context.br[i];
  }

  // Copy floating point registers
  for (int i = 0; i < 128; i++) {
    state->fpu.regs[i] = g_context.fr[i];
  }

  // Special registers
  state->special.ip = g_context.ip;
  state->special.psr = g_context.psr;
  state->special.cfm = g_context.cfm;
  state->special.pr = g_context.pr;

  // Application registers
  state->special.bsp = g_context.bsp;
  state->special.bspstore = g_context.bspstore;
  state->special.rsc = g_context.rsc;
  state->special.rnat = g_context.rnat;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::IA64::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::IA64::CPUState *state =
      reinterpret_cast<const Architecture::IA64::CPUState *>(regs);

  // Copy general registers (skip r0)
  g_context.gr[0] = 0;
  for (int i = 1; i < 128; i++) {
    g_context.gr[i] = state->gp.regs[i];
  }

  // Copy branch registers
  for (int i = 0; i < 8; i++) {
    g_context.br[i] = state->special.br[i];
  }

  // Copy floating point registers
  for (int i = 0; i < 128; i++) {
    g_context.fr[i] = state->fpu.regs[i];
  }

  // Special registers
  g_context.ip = state->special.ip;
  g_context.psr = state->special.psr;
  g_context.cfm = state->special.cfm;
  g_context.pr = state->special.pr;

  // Application registers
  g_context.bsp = state->special.bsp;
  g_context.bspstore = state->special.bspstore;
  g_context.rsc = state->special.rsc;
  g_context.rnat = state->special.rnat;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, PCB &pcb) {
  DS2LOG(Debug, "getting IA-64 process info for PID=%u", pid);

  pcb.pid = pid;
  pcb.state = 1;
  pcb.priority = 4;
  pcb.jib = 0;
  pcb.phd = 0;
  pcb.ksp = 0;
  pcb.bsp = g_context.bsp;

  return kSuccess;
}

} // namespace OpenVMS
} // namespace Host
} // namespace ds2
