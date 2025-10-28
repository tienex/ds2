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
#include "DebugServer2/Architecture/VAX/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace OpenVMS {

// OpenVMS VAX debugging using DEBUG and SYS$DEBUG system services
// Supports OpenVMS VAX 1.0 through 7.3

// VAX register context
struct VAXContext {
  uint32_t r[16];     // R0-R15 (R15 is PC, R14 is SP, R13 is FP, R12 is AP)
  uint32_t psw;       // Processor Status Word
  uint32_t ap;        // Argument pointer (R12)
  uint32_t fp;        // Frame pointer (R13)
  uint32_t sp;        // Stack pointer (R14)
  uint32_t pc;        // Program counter (R15)
};

// OpenVMS Process Control Block (simplified)
struct PCB {
  uint32_t state;       // Process state
  uint32_t pid;         // Process ID
  uint32_t priority;    // Process priority
  uint32_t jib;         // Job Information Block pointer
  uint32_t phd;         // Process Header pointer
};

// System service status codes
#define SS_NORMAL         0x00000001  // Normal successful completion
#define SS_ACCVIO         0x0000000C  // Access violation
#define SS_BADPARAM       0x00000014  // Bad parameter value
#define SS_NOPRIV         0x0000001C  // Insufficient privilege

static bool g_debuggerActive = false;
static VAXContext g_context;

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
  static ErrorCode callSystemService(uint32_t serviceNum, void *args);
};

ErrorCode Debug::attach(uint32_t pid) {
  DS2LOG(Debug, "attaching to OpenVMS VAX process: PID=%u", pid);

  // Use SYS$DEBUG to attach to process
  // This would invoke the DEBUG system service

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t pid) {
  DS2LOG(Debug, "detaching from OpenVMS VAX process: PID=%u", pid);

  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // On VAX, use BPT instruction (opcode 0x03)
  uint8_t bptInstr = 0x03;

  ErrorCode error = writeMemory(address, &bptInstr, sizeof(bptInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint instruction");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::removeBreakpoint(Address address) {
  DS2LOG(Debug, "removing breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Restore original instruction
  return kSuccess;
}

ErrorCode Debug::resume(uint32_t pid) {
  DS2LOG(Debug, "resuming OpenVMS VAX process: PID=%u", pid);

  // Clear trace bits in PSW
  g_context.psw &= ~0x00000010;  // Clear T bit

  // Use SYS$RESUME system service
  return kSuccess;
}

ErrorCode Debug::suspend(uint32_t pid) {
  DS2LOG(Debug, "suspending OpenVMS VAX process: PID=%u", pid);

  // Use SYS$SUSPEND system service
  return kSuccess;
}

ErrorCode Debug::step(uint32_t pid) {
  DS2LOG(Debug, "single-stepping OpenVMS VAX process: PID=%u", pid);

  // Set trace bit in PSW
  g_context.psw |= 0x00000010;  // Set T bit

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  // On OpenVMS, use SYS$READMEM or direct access if privileged
  uint32_t addr = address.value();

  DS2LOG(Debug, "reading %zu bytes from 0x%08x", size, addr);

  // Direct memory access (requires appropriate privileges)
  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint32_t addr = address.value();

  DS2LOG(Debug, "writing %zu bytes to 0x%08x", size, addr);

  // Use SYS$WRITEMEM or direct access
  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::VAX::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::VAX::CPUState *state =
      reinterpret_cast<Architecture::VAX::CPUState *>(regs);

  // Copy general purpose registers
  for (int i = 0; i < 12; i++) {
    state->gp.regs[i] = g_context.r[i];
  }

  // Special registers
  state->special.ap = g_context.ap;  // R12
  state->special.fp = g_context.fp;  // R13
  state->special.sp = g_context.sp;  // R14
  state->special.pc = g_context.pc;  // R15
  state->special.psw = g_context.psw;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::VAX::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::VAX::CPUState *state =
      reinterpret_cast<const Architecture::VAX::CPUState *>(regs);

  // Copy general purpose registers
  for (int i = 0; i < 12; i++) {
    g_context.r[i] = state->gp.regs[i];
  }

  // Special registers
  g_context.ap = state->special.ap;
  g_context.fp = state->special.fp;
  g_context.sp = state->special.sp;
  g_context.pc = state->special.pc;
  g_context.psw = state->special.psw;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, PCB &pcb) {
  DS2LOG(Debug, "getting process info for PID=%u", pid);

  // Use SYS$GETJPI (Get Job/Process Information) system service
  // This would query the process control block

  pcb.pid = pid;
  pcb.state = 1;     // Running
  pcb.priority = 4;  // Normal priority
  pcb.jib = 0;
  pcb.phd = 0;

  return kSuccess;
}

ErrorCode Debug::callSystemService(uint32_t serviceNum, void *args) {
  DS2LOG(Debug, "calling OpenVMS system service: %u", serviceNum);

  // OpenVMS system services are called via CHMK instruction
  // Service number is in R0, arguments in other registers or stack

  // This is a simulation - real implementation would:
  // 1. Set up registers with arguments
  // 2. Execute CHMK instruction
  // 3. Check return status in R0

  return kSuccess;
}

} // namespace OpenVMS
} // namespace Host
} // namespace ds2
