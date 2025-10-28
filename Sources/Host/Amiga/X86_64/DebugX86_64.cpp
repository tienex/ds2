//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86_64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <vector>

namespace ds2 {
namespace Host {
namespace Amiga {

// AROS debugging for x86-64
// AROS x86-64 port provides native 64-bit AmigaOS-like environment

// x86-64 register context
struct X86_64Context {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rsi, rdi;
  uint64_t rbp, rsp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t rip;
  uint64_t rflags;
  uint16_t cs, ds, es, fs, gs, ss;
};

// AROS Task structure (64-bit)
struct AROSTask {
  uint8_t type;
  int8_t priority;
  uint64_t name;
  uint64_t stackPointer;
  uint64_t stackLower;
  uint64_t stackUpper;
  std::string taskName;
};

static bool g_debuggerActive = false;
static X86_64Context g_context;

class Debug {
public:
  static ErrorCode attach(uint64_t taskAddr);
  static ErrorCode detach(uint64_t taskAddr);

  static ErrorCode setBreakpoint(Address address);
  static ErrorCode removeBreakpoint(Address address);

  static ErrorCode resume();
  static ErrorCode suspend();
  static ErrorCode step();

  static ErrorCode readMemory(Address address, void *data, size_t size);
  static ErrorCode writeMemory(Address address, void const *data, size_t size);

  static ErrorCode readRegisters(void *regs, size_t size);
  static ErrorCode writeRegisters(void const *regs, size_t size);

  static ErrorCode getTaskInfo(uint64_t taskAddr, AROSTask &task);
};

ErrorCode Debug::attach(uint64_t taskAddr) {
  DS2LOG(Debug, "attaching to AROS x86-64 task at 0x%016llx",
         (unsigned long long)taskAddr);

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint64_t taskAddr) {
  DS2LOG(Debug, "detaching from AROS x86-64 task at 0x%016llx",
         (unsigned long long)taskAddr);
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

ErrorCode Debug::resume() {
  DS2LOG(Debug, "resuming AROS x86-64 task");

  // Clear trap flag
  g_context.rflags &= ~0x100;

  return kSuccess;
}

ErrorCode Debug::suspend() {
  DS2LOG(Debug, "suspending AROS x86-64 task");
  return kSuccess;
}

ErrorCode Debug::step() {
  DS2LOG(Debug, "single-stepping AROS x86-64 task");

  // Set trap flag
  g_context.rflags |= 0x100;

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  void *destPtr = reinterpret_cast<void *>(address.value());
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

  return kSuccess;
}

ErrorCode Debug::getTaskInfo(uint64_t taskAddr, AROSTask &task) {
  // Read AROS task structure (64-bit pointers)
  uint8_t taskData[96];
  ErrorCode error = readMemory(Address(taskAddr), taskData, sizeof(taskData));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read AROS task structure at 0x%016llx",
           (unsigned long long)taskAddr);
    return error;
  }

  // Parse task structure (little-endian)
  const uint8_t *ptr = taskData;

  task.type = *ptr++;
  task.priority = *ptr++;
  ptr += 6;  // Padding for 64-bit alignment

  // Name pointer (64-bit, little-endian)
  task.name = ptr[0] | ((uint64_t)ptr[1] << 8) | ((uint64_t)ptr[2] << 16) |
              ((uint64_t)ptr[3] << 24) | ((uint64_t)ptr[4] << 32) |
              ((uint64_t)ptr[5] << 40) | ((uint64_t)ptr[6] << 48) |
              ((uint64_t)ptr[7] << 56);
  ptr += 8;

  // Skip to stack info
  ptr += 24;

  task.stackPointer = ptr[0] | ((uint64_t)ptr[1] << 8) |
                      ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
                      ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) |
                      ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);
  ptr += 8;

  task.stackLower = ptr[0] | ((uint64_t)ptr[1] << 8) |
                    ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
                    ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) |
                    ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);
  ptr += 8;

  task.stackUpper = ptr[0] | ((uint64_t)ptr[1] << 8) |
                    ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
                    ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) |
                    ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);

  // Read task name
  if (task.name != 0) {
    char nameBuf[32];
    error = readMemory(Address(task.name), nameBuf, sizeof(nameBuf));
    if (error == kSuccess) {
      task.taskName = std::string(nameBuf, strnlen(nameBuf, 32));
    }
  }

  DS2LOG(Debug, "AROS x86-64 Task: '%s', priority=%d, stack=0x%016llx-0x%016llx",
         task.taskName.c_str(), task.priority,
         (unsigned long long)task.stackLower,
         (unsigned long long)task.stackUpper);

  return kSuccess;
}

} // namespace Amiga
} // namespace Host
} // namespace ds2
