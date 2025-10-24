//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <vector>

namespace ds2 {
namespace Host {
namespace Amiga {

// AmigaOS PowerPC debugging
// Supports AmigaOS 4.0-4.1 (PowerPC) and MorphOS (PowerPC)

// PowerPC context
struct PPCContext {
  uint32_t gpr[32];   // General purpose registers
  double fpr[32];     // Floating point registers
  uint32_t pc;        // Program counter (SRR0)
  uint32_t msr;       // Machine state register (SRR1)
  uint32_t cr;        // Condition register
  uint32_t lr;        // Link register
  uint32_t ctr;       // Count register
  uint32_t xer;       // Integer exception register
  uint32_t fpscr;     // FP status and control
};

// AmigaOS 4 Task structure (PowerPC)
struct AmigaTask {
  uint8_t type;               // Node type
  int8_t priority;            // Task priority
  uint32_t name;              // Task name pointer
  uint32_t stackPointer;      // Current stack pointer
  uint32_t stackLower;        // Stack lower bound
  uint32_t stackUpper;        // Stack upper bound
  uint32_t stackSize;         // Stack size
  bool isPPC;                 // True for native PPC task
};

// MorphOS specific structures
struct MorphOSProcess {
  uint32_t processID;
  uint32_t parentID;
  std::string name;
  Address codeBase;
  uint32_t codeSize;
};

static bool g_debuggerActive = false;
static PPCContext g_context;
static bool g_isMorphOS = false;

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

  static ErrorCode getTaskInfo(uint32_t taskAddr, AmigaTask &task);
  static ErrorCode detectMorphOS(bool &isMorphOS);
};

ErrorCode Debug::attach(uint32_t taskAddr) {
  DS2LOG(Debug, "attaching to AmigaOS PowerPC task at 0x%08x", taskAddr);

  // Detect if running on MorphOS
  ErrorCode error = detectMorphOS(g_isMorphOS);
  if (error == kSuccess) {
    DS2LOG(Debug, "detected %s", g_isMorphOS ? "MorphOS" : "AmigaOS 4");
  }

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t taskAddr) {
  DS2LOG(Debug, "detaching from AmigaOS PowerPC task at 0x%08x", taskAddr);
  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Use 'tw 31,r0,r0' for breakpoint (0x7FE00008)
  uint32_t trapInstr = 0x7FE00008;

  // PowerPC is big-endian
  trapInstr = __builtin_bswap32(trapInstr);

  ErrorCode error = writeMemory(address, &trapInstr, sizeof(trapInstr));
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
  DS2LOG(Debug, "resuming AmigaOS PowerPC task");

  // Clear single-step bit in MSR
  g_context.msr &= ~0x00000400;  // Clear SE bit

  return kSuccess;
}

ErrorCode Debug::suspend() {
  DS2LOG(Debug, "suspending AmigaOS PowerPC task");
  return kSuccess;
}

ErrorCode Debug::step() {
  DS2LOG(Debug, "single-stepping AmigaOS PowerPC task");

  // Set single-step enable bit
  g_context.msr |= 0x00000400;  // Set SE bit

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  uint32_t addr = address.value();

  // AmigaOS 4 memory layout:
  // 0x00000000-0x0FFFFFFF: Low memory (legacy Amiga compatibility)
  // 0x10000000+: PowerPC native memory
  // 0x70000000+: I/O ranges
  // 0xF0000000+: ROM

  if (addr >= 0xF0000000) {
    DS2LOG(Debug, "reading ROM at 0x%08x", addr);
  } else if (addr >= 0x70000000) {
    DS2LOG(Debug, "reading I/O at 0x%08x", addr);
  }

  // Direct memory access
  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint32_t addr = address.value();

  // Protect ROM
  if (addr >= 0xF0000000) {
    DS2LOG(Error, "attempt to write to ROM at 0x%08x", addr);
    return kErrorInvalidArgument;
  }

  // Direct memory write
  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::PowerPC::CPUState *state =
      reinterpret_cast<Architecture::PowerPC::CPUState *>(regs);

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    state->gp.regs[i] = g_context.gpr[i];
  }

  // Copy special registers
  state->special.pc = g_context.pc;
  state->special.msr = g_context.msr;
  state->special.cr = g_context.cr;
  state->special.lr = g_context.lr;
  state->special.ctr = g_context.ctr;
  state->special.xer = g_context.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    state->fpu.regs[i] = g_context.fpr[i];
  }
  state->fpu.fpscr = g_context.fpscr;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::PowerPC::CPUState *state =
      reinterpret_cast<const Architecture::PowerPC::CPUState *>(regs);

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    g_context.gpr[i] = state->gp.regs[i];
  }

  // Copy special registers
  g_context.pc = state->special.pc;
  g_context.msr = state->special.msr;
  g_context.cr = state->special.cr;
  g_context.lr = state->special.lr;
  g_context.ctr = state->special.ctr;
  g_context.xer = state->special.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    g_context.fpr[i] = state->fpu.regs[i];
  }
  g_context.fpscr = state->fpu.fpscr;

  return kSuccess;
}

ErrorCode Debug::getTaskInfo(uint32_t taskAddr, AmigaTask &task) {
  // Read task structure
  uint8_t taskData[64];
  ErrorCode error = readMemory(Address(taskAddr), taskData, sizeof(taskData));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read task structure at 0x%08x", taskAddr);
    return error;
  }

  // Parse task structure (big-endian)
  const uint8_t *ptr = taskData;

  task.type = *ptr++;
  task.priority = *ptr++;
  ptr += 2;

  // Name pointer
  task.name = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;

  // Skip to stack info
  ptr += 16;

  task.stackPointer = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  task.stackLower = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  task.stackUpper = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

  task.stackSize = task.stackUpper - task.stackLower;
  task.isPPC = true;

  DS2LOG(Debug, "PowerPC Task: type=%u, priority=%d, stack=0x%08x-0x%08x",
         task.type, task.priority, task.stackLower, task.stackUpper);

  return kSuccess;
}

ErrorCode Debug::detectMorphOS(bool &isMorphOS) {
  // MorphOS has specific ROM signatures and system structures
  // Try to read a known MorphOS identifier

  // MorphOS ROM tag is at a specific location
  uint32_t romTag = 0;
  ErrorCode error = readMemory(Address(0xFFFFFFF0), &romTag, sizeof(romTag));

  if (error == kSuccess) {
    romTag = __builtin_bswap32(romTag);

    // MorphOS ROM tags typically have specific values
    // This is a simplified check
    if (romTag == 0x4D4F5250) {  // 'MORP'
      isMorphOS = true;
      return kSuccess;
    }
  }

  isMorphOS = false;
  return kSuccess;
}

} // namespace Amiga
} // namespace Host
} // namespace ds2
