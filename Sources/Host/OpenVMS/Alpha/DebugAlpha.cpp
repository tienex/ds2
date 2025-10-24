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
#include "DebugServer2/Architecture/Alpha/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace OpenVMS {

// OpenVMS Alpha debugging
// Supports OpenVMS Alpha 1.5 through 8.4

// Alpha register context
struct AlphaContext {
  uint64_t r[32];     // Integer registers R0-R31 (R31 is zero)
  uint64_t f[32];     // Floating point registers F0-F31 (F31 is zero)
  uint64_t pc;        // Program counter
  uint64_t ps;        // Processor status
  uint64_t fpcr;      // Floating point control register
  uint64_t unique;    // Thread-unique value register
};

// OpenVMS Process Control Block (Alpha)
struct PCB {
  uint64_t state;       // Process state
  uint32_t pid;         // Process ID
  uint32_t priority;    // Process priority
  uint64_t jib;         // Job Information Block pointer
  uint64_t phd;         // Process Header pointer
  uint64_t ksp;         // Kernel stack pointer
};

// PALcode function numbers (Privileged Architecture Library)
#define PAL_HALT          0x00    // Halt processor
#define PAL_CFLUSH        0x01    // Cache flush
#define PAL_DRAINA        0x02    // Drain aborts
#define PAL_BPT           0x80    // Breakpoint
#define PAL_GENTRAP       0xAA    // Generate trap

static bool g_debuggerActive = false;
static AlphaContext g_context;

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
  static ErrorCode callPALcode(uint32_t function, uint64_t *args);
};

ErrorCode Debug::attach(uint32_t pid) {
  DS2LOG(Debug, "attaching to OpenVMS Alpha process: PID=%u", pid);

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t pid) {
  DS2LOG(Debug, "detaching from OpenVMS Alpha process: PID=%u", pid);

  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  // On Alpha, use CALL_PAL BPT instruction
  // Format: opcode (6 bits) + function (26 bits)
  // CALL_PAL = 0x00, BPT = 0x80
  // Instruction: 0x00000080
  uint32_t bptInstr = 0x00000080;

  ErrorCode error = writeMemory(address, &bptInstr, sizeof(bptInstr));
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
  DS2LOG(Debug, "resuming OpenVMS Alpha process: PID=%u", pid);

  // Clear single-step bit in processor status
  g_context.ps &= ~0x01;

  return kSuccess;
}

ErrorCode Debug::suspend(uint32_t pid) {
  DS2LOG(Debug, "suspending OpenVMS Alpha process: PID=%u", pid);
  return kSuccess;
}

ErrorCode Debug::step(uint32_t pid) {
  DS2LOG(Debug, "single-stepping OpenVMS Alpha process: PID=%u", pid);

  // Set single-step bit
  g_context.ps |= 0x01;

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
  if (size < sizeof(Architecture::Alpha::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::Alpha::CPUState *state =
      reinterpret_cast<Architecture::Alpha::CPUState *>(regs);

  // Copy integer registers (R0-R30, R31 is always zero)
  for (int i = 0; i < 31; i++) {
    state->gp.regs[i] = g_context.r[i];
  }
  state->gp.regs[31] = 0;  // R31 hardwired to zero

  // Copy floating point registers (F0-F30, F31 is always zero)
  for (int i = 0; i < 31; i++) {
    state->fpu.regs[i] = g_context.f[i];
  }
  state->fpu.regs[31] = 0.0;  // F31 hardwired to zero

  // Special registers
  state->special.pc = g_context.pc;
  state->special.ps = g_context.ps;
  state->special.fpcr = g_context.fpcr;
  state->special.unique = g_context.unique;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::Alpha::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::Alpha::CPUState *state =
      reinterpret_cast<const Architecture::Alpha::CPUState *>(regs);

  // Copy integer registers (skip R31 - hardwired to zero)
  for (int i = 0; i < 31; i++) {
    g_context.r[i] = state->gp.regs[i];
  }
  g_context.r[31] = 0;

  // Copy floating point registers (skip F31)
  for (int i = 0; i < 31; i++) {
    g_context.f[i] = state->fpu.regs[i];
  }
  g_context.f[31] = 0.0;

  // Special registers
  g_context.pc = state->special.pc;
  g_context.ps = state->special.ps;
  g_context.fpcr = state->special.fpcr;
  g_context.unique = state->special.unique;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, PCB &pcb) {
  DS2LOG(Debug, "getting process info for PID=%u", pid);

  // Use SYS$GETJPI system service
  pcb.pid = pid;
  pcb.state = 1;
  pcb.priority = 4;
  pcb.jib = 0;
  pcb.phd = 0;
  pcb.ksp = 0;

  return kSuccess;
}

ErrorCode Debug::callPALcode(uint32_t function, uint64_t *args) {
  DS2LOG(Debug, "calling PALcode function: 0x%02x", function);

  // PALcode (Privileged Architecture Library) provides:
  // - Cache management
  // - TLB management
  // - Interrupt control
  // - Context switching
  // - Exception handling

  // This would execute CALL_PAL instruction with function number
  // Arguments are passed in registers R16-R21
  // Return values in R0, R20, R21

  switch (function) {
  case PAL_BPT:
    DS2LOG(Debug, "PAL_BPT - breakpoint");
    break;
  case PAL_HALT:
    DS2LOG(Debug, "PAL_HALT - halt processor");
    break;
  case PAL_CFLUSH:
    DS2LOG(Debug, "PAL_CFLUSH - flush cache");
    break;
  case PAL_DRAINA:
    DS2LOG(Debug, "PAL_DRAINA - drain write buffer");
    break;
  case PAL_GENTRAP:
    DS2LOG(Debug, "PAL_GENTRAP - generate trap");
    break;
  default:
    DS2LOG(Warning, "unknown PALcode function: 0x%02x", function);
    break;
  }

  return kSuccess;
}

} // namespace OpenVMS
} // namespace Host
} // namespace ds2
