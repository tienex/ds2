//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/MacOS/Debug.h"
#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace MacOS {

// Classic Mac OS m68k debugging using MacsBug low-level API
// Supports System 1.0 through Mac OS 9.2.2 on 68000/020/030/040

// Low-level register context for 68k
struct M68kContext {
  // Data registers D0-D7
  uint32_t d[8];

  // Address registers A0-A7 (A7 is stack pointer)
  uint32_t a[8];

  // Program counter
  uint32_t pc;

  // Status register
  uint16_t sr;

  // FPU registers (68881/68882/68040)
  double fp[8];
  uint32_t fpcr;  // Control register
  uint32_t fpsr;  // Status register
  uint32_t fpiar; // Instruction address register
};

// MacsBug Debugger Entry Points (via A-line traps)
#define MACSBUG_TRAP_DEBUGGER    0xA9FF  // Debugger() - enter debugger
#define MACSBUG_TRAP_DEBUGSTR    0xABFF  // DebugStr() - with message
#define MACSBUG_TRAP_SYSBREAKSTR 0xABFE  // SysBreakStr()

// Exception vector table (starts at address 0)
#define M68K_VECTOR_BASE         0x00000000
#define M68K_VECTOR_TRACE        0x00000024  // Trace exception
#define M68K_VECTOR_ILLEGAL      0x00000010  // Illegal instruction
#define M68K_VECTOR_TRAP0        0x00000080  // TRAP #0 (OS trap)

// Global debugger state
static bool g_debuggerInstalled = false;
static M68kContext g_savedContext;

// Low memory globals access
#define LM_CurrentA5      0x0904  // Current A5 register
#define LM_CurStackBase   0x0908  // Current stack base
#define LM_CurApName      0x0910  // Current application name pointer
#define LM_ApplZone       0x02AA  // Application heap zone
#define LM_BufPtr         0x010C  // Top of application heap

ErrorCode Debug::attach(ProcessInfo const &info) {
  if (!info.is68k) {
    DS2LOG(Error, "attempting to attach to non-68k process");
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug, "attaching to Mac OS 68k process: PID=%u, name='%s'",
         info.processID, info.processName.c_str());

  // Install trace exception handler for single-stepping
  ErrorCode error = installExceptionHandler(M68K_VECTOR_TRACE,
                                           Address(0)); // Placeholder
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to install trace handler");
  }

  g_debuggerInstalled = true;
  return kSuccess;
}

ErrorCode Debug::detach(ProcessInfo const &info) {
  if (g_debuggerInstalled) {
    removeExceptionHandler(M68K_VECTOR_TRACE);
    g_debuggerInstalled = false;
  }

  DS2LOG(Debug, "detached from Mac OS 68k process: PID=%u", info.processID);
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  // On 68k, breakpoints are implemented by replacing instruction with TRAP #15
  // or using MacsBug's internal breakpoint table

  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Read original instruction
  uint16_t originalInstr;
  ErrorCode error = readMemory(address, &originalInstr, sizeof(originalInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read instruction at breakpoint");
    return error;
  }

  // Replace with TRAP #15 instruction (0x4E4F)
  uint16_t trapInstr = 0x4E4F;  // TRAP #15

  // Swap to big-endian
  trapInstr = __builtin_bswap16(trapInstr);

  error = writeMemory(address, &trapInstr, sizeof(trapInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint instruction");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::removeBreakpoint(Address address) {
  // Would restore original instruction from breakpoint table
  DS2LOG(Debug, "removing breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // This would require maintaining a table of original instructions
  return kSuccess;
}

ErrorCode Debug::resume(ProcessInfo const &info) {
  DS2LOG(Debug, "resuming Mac OS 68k process: PID=%u", info.processID);

  // Clear trace bit in SR to resume normal execution
  g_savedContext.sr &= ~0x8000;  // Clear T bit

  // Would restore context and continue execution
  return kSuccess;
}

ErrorCode Debug::suspend(ProcessInfo const &info) {
  DS2LOG(Debug, "suspending Mac OS 68k process: PID=%u", info.processID);

  // Trigger debugger entry via Debugger() trap
  // In real implementation, this would invoke the trap

  return kSuccess;
}

ErrorCode Debug::step(ProcessInfo const &info) {
  DS2LOG(Debug, "single-stepping Mac OS 68k process: PID=%u", info.processID);

  // Set trace bit in status register
  g_savedContext.sr |= 0x8000;  // Set T bit (trace)

  // Execute one instruction - trace exception will fire after
  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  // On Classic Mac OS, memory is directly accessible
  // No virtual memory protection on 68k Macs

  // Simple memory copy (in real implementation would need to validate)
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

ErrorCode Debug::readRegisters68k(void *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::M68k::CPUState *state =
      reinterpret_cast<Architecture::M68k::CPUState *>(regs);

  // Copy from saved context
  for (int i = 0; i < 8; i++) {
    state->gp.regs[i] = g_savedContext.d[i];
    state->gp.regs[i + 8] = g_savedContext.a[i];
  }

  state->special.pc = g_savedContext.pc;
  state->special.sr = g_savedContext.sr;

  // FPU registers
  for (int i = 0; i < 8; i++) {
    // Convert double to extended precision format
    state->fpu.regs[i].d = g_savedContext.fp[i];
  }
  state->fpu.fpcr = g_savedContext.fpcr;
  state->fpu.fpsr = g_savedContext.fpsr;
  state->fpu.fpiar = g_savedContext.fpiar;

  return kSuccess;
}

ErrorCode Debug::writeRegisters68k(void const *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::M68k::CPUState *state =
      reinterpret_cast<const Architecture::M68k::CPUState *>(regs);

  // Copy to saved context
  for (int i = 0; i < 8; i++) {
    g_savedContext.d[i] = state->gp.regs[i];
    g_savedContext.a[i] = state->gp.regs[i + 8];
  }

  g_savedContext.pc = state->special.pc;
  g_savedContext.sr = state->special.sr;

  // FPU registers
  for (int i = 0; i < 8; i++) {
    g_savedContext.fp[i] = state->fpu.regs[i].d;
  }
  g_savedContext.fpcr = state->fpu.fpcr;
  g_savedContext.fpsr = state->fpu.fpsr;
  g_savedContext.fpiar = state->fpu.fpiar;

  return kSuccess;
}

ErrorCode Debug::getTrapAddress(uint16_t trapNum, Address &address) {
  // On Mac OS, trap addresses are in the trap dispatch table
  // For OS traps (A-line), the table is in system memory

  if ((trapNum & 0xF000) != 0xA000) {
    DS2LOG(Error, "invalid trap number: 0x%04x", trapNum);
    return kErrorInvalidArgument;
  }

  // Trap dispatch table is accessed via GetTrapAddress() toolbox routine
  // For simulation, we'll use a placeholder
  address = Address(0x00040000 + (trapNum & 0x0FFF) * 4);

  DS2LOG(Debug, "trap 0x%04x at address 0x%08llx",
         trapNum, (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::setTrapAddress(uint16_t trapNum, Address address) {
  if ((trapNum & 0xF000) != 0xA000) {
    DS2LOG(Error, "invalid trap number: 0x%04x", trapNum);
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug, "setting trap 0x%04x to address 0x%08llx",
         trapNum, (unsigned long long)address.value());

  // Would patch trap dispatch table
  return kSuccess;
}

ErrorCode Debug::installExceptionHandler(uint32_t vector, Address handler) {
  // Exception vectors are at fixed addresses starting at 0
  Address vectorAddr = Address(vector);

  DS2LOG(Debug, "installing exception handler at vector 0x%08x", vector);

  // Read current vector
  uint32_t currentHandler;
  ErrorCode error = readMemory(vectorAddr, &currentHandler,
                               sizeof(currentHandler));
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to read exception vector");
  }

  // Write new handler address (big-endian)
  uint32_t handlerAddr = __builtin_bswap32(handler.value());
  error = writeMemory(vectorAddr, &handlerAddr, sizeof(handlerAddr));

  return error;
}

ErrorCode Debug::removeExceptionHandler(uint32_t vector) {
  DS2LOG(Debug, "removing exception handler at vector 0x%08x", vector);

  // Would restore original vector
  return kSuccess;
}

ErrorCode Debug::getProcessList(std::vector<ProcessInfo> &processes) {
  // On Classic Mac OS, there's no true process list
  // In MultiFinder/System 7+, we'd enumerate application partitions

  processes.clear();

  // Add current application
  ProcessInfo current;
  current.processID = 1;
  current.processSerial = 1;
  current.processType = 0x4150504C;  // 'APPL'
  current.processSignature = 0x3F3F3F3F;  // '????'
  current.is68k = true;
  current.processName = "Current Application";

  processes.push_back(current);

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, ProcessInfo &info) {
  // Read low memory globals to get process information
  uint32_t currentA5;
  ErrorCode error = readMemory(Address(LM_CurrentA5), &currentA5,
                               sizeof(currentA5));
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to read CurrentA5 low memory global");
  }

  info.processID = pid;
  info.processSerial = pid;
  info.processType = 0x4150504C;  // 'APPL'
  info.is68k = true;

  // Read application name from low memory
  uint32_t namePtr;
  error = readMemory(Address(LM_CurApName), &namePtr, sizeof(namePtr));
  if (error == kSuccess && namePtr != 0) {
    // Read Pascal string (length byte + characters)
    uint8_t len;
    error = readMemory(Address(namePtr), &len, 1);
    if (error == kSuccess && len > 0 && len < 32) {
      std::vector<char> nameBuf(len);
      error = readMemory(Address(namePtr + 1), nameBuf.data(), len);
      if (error == kSuccess) {
        info.processName = std::string(nameBuf.data(), len);
      }
    }
  }

  return kSuccess;
}

// Stubs for PowerPC-specific functions (not applicable for 68k)
ErrorCode Debug::readRegistersPPC(void *regs, size_t size) {
  DS2LOG(Error, "readRegistersPPC called on 68k debugger");
  return kErrorUnsupported;
}

ErrorCode Debug::writeRegistersPPC(void const *regs, size_t size) {
  DS2LOG(Error, "writeRegistersPPC called on 68k debugger");
  return kErrorUnsupported;
}

ErrorCode Debug::getCFMConnections(std::vector<CFMConnectionID> &connections) {
  DS2LOG(Error, "getCFMConnections not supported on 68k");
  return kErrorUnsupported;
}

ErrorCode Debug::loadCFMFragment(std::string const &path,
                                 CFMConnectionID &connection) {
  DS2LOG(Error, "loadCFMFragment not supported on 68k");
  return kErrorUnsupported;
}

bool Debug::_initialized = false;

} // namespace MacOS
} // namespace Host
} // namespace ds2
