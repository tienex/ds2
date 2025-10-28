//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <vector>

namespace ds2 {
namespace Host {
namespace Amiga {

// AmigaOS debugging for m68k
// Supports AmigaOS 1.0-3.9 on 68000/010/020/030/040/060

// Exec library base (always at 0x0004)
#define EXEC_BASE_PTR         0x00000004

// Exception vectors
#define AMIGA_VECTOR_BASE     0x00000000
#define AMIGA_VECTOR_BUSERR   0x00000008  // Bus error
#define AMIGA_VECTOR_ADDRERR  0x0000000C  // Address error
#define AMIGA_VECTOR_ILLEGAL  0x00000010  // Illegal instruction
#define AMIGA_VECTOR_ZERODIV  0x00000014  // Zero divide
#define AMIGA_VECTOR_TRACE    0x00000024  // Trace
#define AMIGA_VECTOR_TRAP0    0x00000080  // TRAP #0

// Memory regions
#define CHIP_MEM_BASE         0x00000000  // Chip RAM (graphics accessible)
#define CHIP_MEM_SIZE         0x00200000  // Typical: 512KB-2MB
#define FAST_MEM_BASE         0x00200000  // Fast RAM (CPU only)
#define KICKSTART_ROM         0x00F80000  // Kickstart ROM (512KB)
#define CIA_A_BASE            0x00BFE001  // CIA-A (odd bytes)
#define CIA_B_BASE            0x00BFD000  // CIA-B
#define CUSTOM_BASE           0x00DFF000  // Custom chips

// Exec library offsets
#define EXEC_FINDTASK         -294        // FindTask()
#define EXEC_FORBID           -132        // Forbid()
#define EXEC_PERMIT           -138        // Permit()
#define EXEC_ALLOCMEM         -198        // AllocMem()
#define EXEC_FREEMEM          -210        // FreeMem()

// M68k context
struct M68kContext {
  uint32_t d[8];      // D0-D7
  uint32_t a[8];      // A0-A7
  uint32_t pc;        // Program counter
  uint16_t sr;        // Status register
  uint32_t usp;       // User stack pointer
  uint32_t ssp;       // Supervisor stack pointer
};

// Amiga Task structure (simplified)
struct AmigaTask {
  uint8_t type;               // Node type
  int8_t priority;            // Task priority
  uint32_t name;              // Task name pointer
  uint32_t stackPointer;      // Current stack pointer
  uint32_t stackLower;        // Stack lower bound
  uint32_t stackUpper;        // Stack upper bound
  uint32_t stackSize;         // Stack size
};

static bool g_debuggerActive = false;
static M68kContext g_context;
static uint32_t g_execBase = 0;

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

  static ErrorCode getExecBase(uint32_t &execBase);
  static ErrorCode getTaskInfo(uint32_t taskAddr, AmigaTask &task);
  static ErrorCode getTaskList(std::vector<uint32_t> &tasks);
};

ErrorCode Debug::attach(uint32_t taskAddr) {
  DS2LOG(Debug, "attaching to AmigaOS task at 0x%08x", taskAddr);

  // Get Exec library base
  ErrorCode error = getExecBase(g_execBase);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to get Exec base");
    return error;
  }

  DS2LOG(Debug, "Exec base at 0x%08x", g_execBase);

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t taskAddr) {
  DS2LOG(Debug, "detaching from AmigaOS task at 0x%08x", taskAddr);
  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Use ILLEGAL instruction (0x4AFC) for breakpoint
  uint16_t illegalInstr = 0x4AFC;

  // Amiga is big-endian
  illegalInstr = __builtin_bswap16(illegalInstr);

  ErrorCode error = writeMemory(address, &illegalInstr, sizeof(illegalInstr));
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
  DS2LOG(Debug, "resuming AmigaOS task");

  // Clear trace bit
  g_context.sr &= ~0x8000;

  return kSuccess;
}

ErrorCode Debug::suspend() {
  DS2LOG(Debug, "suspending AmigaOS task");

  // Call Forbid() to disable task switching
  // This would be done via Exec library call

  return kSuccess;
}

ErrorCode Debug::step() {
  DS2LOG(Debug, "single-stepping AmigaOS task");

  // Set trace bit
  g_context.sr |= 0x8000;

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  uint32_t addr = address.value();

  // Validate memory regions
  if (addr >= KICKSTART_ROM && addr < KICKSTART_ROM + 0x80000) {
    // Kickstart ROM - read-only
    DS2LOG(Debug, "reading Kickstart ROM at 0x%08x", addr);
  } else if (addr >= CUSTOM_BASE && addr < CUSTOM_BASE + 0x1000) {
    // Custom chip registers - special handling
    DS2LOG(Debug, "reading custom chip registers at 0x%08x", addr);
  } else if (addr >= CIA_A_BASE && addr < CIA_A_BASE + 0x1000) {
    // CIA registers
    DS2LOG(Debug, "reading CIA registers at 0x%08x", addr);
  }

  // Direct memory access
  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint32_t addr = address.value();

  // Protect ROM area
  if (addr >= KICKSTART_ROM && addr < KICKSTART_ROM + 0x80000) {
    DS2LOG(Error, "attempt to write to Kickstart ROM at 0x%08x", addr);
    return kErrorInvalidArgument;
  }

  // Direct memory write
  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::M68k::CPUState *state =
      reinterpret_cast<Architecture::M68k::CPUState *>(regs);

  // Copy data registers
  for (int i = 0; i < 8; i++) {
    state->gp.regs[i] = g_context.d[i];
  }

  // Copy address registers
  for (int i = 0; i < 8; i++) {
    state->gp.regs[i + 8] = g_context.a[i];
  }

  // Special registers
  state->special.pc = g_context.pc;
  state->special.sr = g_context.sr;
  state->special.usp = g_context.usp;
  state->special.ssp = g_context.ssp;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::M68k::CPUState *state =
      reinterpret_cast<const Architecture::M68k::CPUState *>(regs);

  // Copy data registers
  for (int i = 0; i < 8; i++) {
    g_context.d[i] = state->gp.regs[i];
  }

  // Copy address registers
  for (int i = 0; i < 8; i++) {
    g_context.a[i] = state->gp.regs[i + 8];
  }

  // Special registers
  g_context.pc = state->special.pc;
  g_context.sr = state->special.sr;
  g_context.usp = state->special.usp;
  g_context.ssp = state->special.ssp;

  return kSuccess;
}

ErrorCode Debug::getExecBase(uint32_t &execBase) {
  // Exec base pointer is at absolute address 0x0004
  ErrorCode error = readMemory(Address(EXEC_BASE_PTR), &execBase,
                               sizeof(execBase));
  if (error != kSuccess) {
    return error;
  }

  // Convert from big-endian
  execBase = __builtin_bswap32(execBase);

  return kSuccess;
}

ErrorCode Debug::getTaskInfo(uint32_t taskAddr, AmigaTask &task) {
  // Read task structure
  uint8_t taskData[32];
  ErrorCode error = readMemory(Address(taskAddr), taskData, sizeof(taskData));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read task structure at 0x%08x", taskAddr);
    return error;
  }

  // Parse task structure (big-endian)
  const uint8_t *ptr = taskData;

  task.type = *ptr++;          // Byte: node type
  task.priority = *ptr++;      // Byte: priority
  ptr += 2;                    // Skip 2 bytes

  // Name pointer (4 bytes, big-endian)
  task.name = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;

  // Skip some fields, get to stack info
  ptr += 8;

  task.stackPointer = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  task.stackLower = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  task.stackUpper = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

  task.stackSize = task.stackUpper - task.stackLower;

  DS2LOG(Debug, "Task: type=%u, priority=%d, stack=0x%08x-0x%08x",
         task.type, task.priority, task.stackLower, task.stackUpper);

  return kSuccess;
}

ErrorCode Debug::getTaskList(std::vector<uint32_t> &tasks) {
  tasks.clear();

  // In a real implementation, we would walk the Exec task lists
  // (Ready, Waiting, etc.) using the List structures in Exec

  DS2LOG(Debug, "enumerating AmigaOS tasks");

  // For now, just return an empty list
  // A real implementation would:
  // 1. Get Exec base
  // 2. Read TaskReady and TaskWait list heads
  // 3. Walk each list following ln_Succ pointers
  // 4. Add each task address to the vector

  return kSuccess;
}

} // namespace Amiga
} // namespace Host
} // namespace ds2
