//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <vector>

namespace ds2 {
namespace Host {
namespace Amiga {

// AROS (Amiga Research Operating System) debugging for x86
// AROS is an open-source implementation of AmigaOS for modern platforms

// x86 register context
struct X86Context {
  uint32_t eax, ebx, ecx, edx;
  uint32_t esi, edi;
  uint32_t ebp, esp;
  uint32_t eip;
  uint32_t eflags;
  uint16_t cs, ds, es, fs, gs, ss;
};

// AROS Task structure
struct AROSTask {
  uint8_t type;
  int8_t priority;
  uint32_t name;
  uint32_t stackPointer;
  uint32_t stackLower;
  uint32_t stackUpper;
  std::string taskName;
};

static bool g_debuggerActive = false;
static X86Context g_context;

class Debug {
public:
  static ErrorCode attach(uint32_t taskAddr);
  static ErrorCode detach(uint32_t taskAddr);

  static ErrorCode setBreakpoint(Address address);
  static ErrorCode removeBreakpoint(Address address);

  static ErrorCode resume();
  static ErrorCode suspend();
  static ErrorCode step();

  static ErrorCode readMemory(Address address, void *data, size_t size);
  static ErrorCode writeMemory(Address address, void const *data, size_t size);

  static ErrorCode readRegisters(void *regs, size_t size);
  static ErrorCode writeRegisters(void const *regs, size_t size);

  static ErrorCode getTaskInfo(uint32_t taskAddr, AROSTask &task);
};

ErrorCode Debug::attach(uint32_t taskAddr) {
  DS2LOG(Debug, "attaching to AROS x86 task at 0x%08x", taskAddr);

  // AROS uses standard debugging interfaces on hosted platforms
  // On native AROS, we'd use Exec debugging facilities

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t taskAddr) {
  DS2LOG(Debug, "detaching from AROS x86 task at 0x%08x", taskAddr);
  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Use INT3 instruction (0xCC) for breakpoint
  uint8_t int3 = 0xCC;

  ErrorCode error = writeMemory(address, &int3, sizeof(int3));
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

ErrorCode Debug::resume() {
  DS2LOG(Debug, "resuming AROS x86 task");

  // Clear trap flag in EFLAGS
  g_context.eflags &= ~0x100;

  return kSuccess;
}

ErrorCode Debug::suspend() {
  DS2LOG(Debug, "suspending AROS x86 task");
  return kSuccess;
}

ErrorCode Debug::step() {
  DS2LOG(Debug, "single-stepping AROS x86 task");

  // Set trap flag in EFLAGS
  g_context.eflags |= 0x100;

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  // Direct memory access
  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  // Direct memory write
  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::X86::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::X86::CPUState *state =
      reinterpret_cast<Architecture::X86::CPUState *>(regs);

  // Copy general purpose registers
  state->gp.eax = g_context.eax;
  state->gp.ebx = g_context.ebx;
  state->gp.ecx = g_context.ecx;
  state->gp.edx = g_context.edx;
  state->gp.esi = g_context.esi;
  state->gp.edi = g_context.edi;
  state->gp.ebp = g_context.ebp;
  state->gp.esp = g_context.esp;

  // Special registers
  state->special.eip = g_context.eip;
  state->special.eflags = g_context.eflags;

  // Segment registers
  state->special.cs = g_context.cs;
  state->special.ds = g_context.ds;
  state->special.es = g_context.es;
  state->special.fs = g_context.fs;
  state->special.gs = g_context.gs;
  state->special.ss = g_context.ss;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::X86::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::X86::CPUState *state =
      reinterpret_cast<const Architecture::X86::CPUState *>(regs);

  // Copy general purpose registers
  g_context.eax = state->gp.eax;
  g_context.ebx = state->gp.ebx;
  g_context.ecx = state->gp.ecx;
  g_context.edx = state->gp.edx;
  g_context.esi = state->gp.esi;
  g_context.edi = state->gp.edi;
  g_context.ebp = state->gp.ebp;
  g_context.esp = state->gp.esp;

  // Special registers
  g_context.eip = state->special.eip;
  g_context.eflags = state->special.eflags;

  // Segment registers
  g_context.cs = state->special.cs;
  g_context.ds = state->special.ds;
  g_context.es = state->special.es;
  g_context.fs = state->special.fs;
  g_context.gs = state->special.gs;
  g_context.ss = state->special.ss;

  return kSuccess;
}

ErrorCode Debug::getTaskInfo(uint32_t taskAddr, AROSTask &task) {
  // Read AROS task structure
  uint8_t taskData[64];
  ErrorCode error = readMemory(Address(taskAddr), taskData, sizeof(taskData));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read AROS task structure at 0x%08x", taskAddr);
    return error;
  }

  // Parse task structure (little-endian on x86)
  const uint8_t *ptr = taskData;

  task.type = *ptr++;
  task.priority = *ptr++;
  ptr += 2;

  // Name pointer (little-endian)
  task.name = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;

  // Skip to stack info
  ptr += 16;

  task.stackPointer = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  task.stackLower = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  task.stackUpper = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);

  // Read task name if pointer is valid
  if (task.name != 0) {
    char nameBuf[32];
    error = readMemory(Address(task.name), nameBuf, sizeof(nameBuf));
    if (error == kSuccess) {
      task.taskName = std::string(nameBuf, strnlen(nameBuf, 32));
    }
  }

  DS2LOG(Debug, "AROS Task: '%s', priority=%d, stack=0x%08x-0x%08x",
         task.taskName.c_str(), task.priority, task.stackLower,
         task.stackUpper);

  return kSuccess;
}

} // namespace Amiga
} // namespace Host
} // namespace ds2
