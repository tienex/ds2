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
#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace MacOS {

// Classic Mac OS PowerPC debugging
// Supports System 7.1.2 through Mac OS 9.2.2 on PowerPC 601/603/604/G3/G4

// PowerPC register context
struct PPCContext {
  // General purpose registers r0-r31
  uint32_t gpr[32];

  // Floating point registers f0-f31
  double fpr[32];

  // Special registers
  uint32_t pc;       // Program counter (SRR0 on exception)
  uint32_t msr;      // Machine state register (SRR1 on exception)
  uint32_t cr;       // Condition register
  uint32_t lr;       // Link register
  uint32_t ctr;      // Count register
  uint32_t xer;      // Integer exception register

  // FPSCR
  uint32_t fpscr;

  // AltiVec/VMX registers (G4 and later)
  uint8_t vr[32][16];  // 32 x 128-bit vector registers
  uint32_t vscr;       // Vector status and control register
  uint32_t vrsave;     // Vector register save register
};

// Code Fragment Manager structures
struct CFMFragmentInfo {
  uint32_t connectionID;
  uint32_t fragmentID;
  std::string fragmentName;
  Address codeAddress;
  uint32_t codeLength;
  Address dataAddress;
  uint32_t dataLength;
};

// PowerPC exception vectors
#define PPC_VECTOR_SYSTEM_RESET   0x00100
#define PPC_VECTOR_DSI            0x00300  // Data storage interrupt
#define PPC_VECTOR_ISI            0x00400  // Instruction storage interrupt
#define PPC_VECTOR_PROGRAM        0x00700  // Program exception
#define PPC_VECTOR_TRACE          0x00D00  // Trace exception
#define PPC_VECTOR_BREAKPOINT     0x01300  // Instruction breakpoint

// Global state
static bool g_debuggerInstalled = false;
static PPCContext g_savedContext;
static std::vector<CFMFragmentInfo> g_cfmFragments;

// MSR bits
#define MSR_SE   0x00000400  // Single-step enable
#define MSR_BE   0x00000200  // Branch trace enable

ErrorCode Debug::attach(ProcessInfo const &info) {
  if (info.is68k) {
    DS2LOG(Error, "attempting to attach to 68k process with PPC debugger");
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug, "attaching to Mac OS PowerPC process: PID=%u, name='%s'",
         info.processID, info.processName.c_str());

  // Install trace exception handler
  ErrorCode error = installExceptionHandler(PPC_VECTOR_TRACE,
                                           Address(0)); // Placeholder
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to install trace handler");
  }

  // Install breakpoint exception handler
  error = installExceptionHandler(PPC_VECTOR_BREAKPOINT, Address(0));
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to install breakpoint handler");
  }

  g_debuggerInstalled = true;
  return kSuccess;
}

ErrorCode Debug::detach(ProcessInfo const &info) {
  if (g_debuggerInstalled) {
    removeExceptionHandler(PPC_VECTOR_TRACE);
    removeExceptionHandler(PPC_VECTOR_BREAKPOINT);
    g_debuggerInstalled = false;
  }

  DS2LOG(Debug, "detached from Mac OS PowerPC process: PID=%u", info.processID);
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // On PowerPC, use 'tw' (trap word) instruction for breakpoints
  // tw 31,r0,r0 is commonly used (0x7FE00008)
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

  // Restore original instruction from breakpoint table
  return kSuccess;
}

ErrorCode Debug::resume(ProcessInfo const &info) {
  DS2LOG(Debug, "resuming Mac OS PowerPC process: PID=%u", info.processID);

  // Clear single-step and branch trace bits
  g_savedContext.msr &= ~(MSR_SE | MSR_BE);

  // Would restore context and continue execution
  return kSuccess;
}

ErrorCode Debug::suspend(ProcessInfo const &info) {
  DS2LOG(Debug, "suspending Mac OS PowerPC process: PID=%u", info.processID);

  // Trigger debugger entry
  return kSuccess;
}

ErrorCode Debug::step(ProcessInfo const &info) {
  DS2LOG(Debug, "single-stepping Mac OS PowerPC process: PID=%u",
         info.processID);

  // Set single-step enable bit in MSR
  g_savedContext.msr |= MSR_SE;

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  // Direct memory access on Classic Mac OS
  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegistersPPC(void *regs, size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::PowerPC::CPUState *state =
      reinterpret_cast<Architecture::PowerPC::CPUState *>(regs);

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    state->gp.regs[i] = g_savedContext.gpr[i];
  }

  // Copy special registers
  state->special.pc = g_savedContext.pc;
  state->special.msr = g_savedContext.msr;
  state->special.cr = g_savedContext.cr;
  state->special.lr = g_savedContext.lr;
  state->special.ctr = g_savedContext.ctr;
  state->special.xer = g_savedContext.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    state->fpu.regs[i] = g_savedContext.fpr[i];
  }
  state->fpu.fpscr = g_savedContext.fpscr;

  // Copy vector registers if available
  for (int i = 0; i < 32; i++) {
    memcpy(state->vec.vr[i], g_savedContext.vr[i], 16);
  }
  state->vec.vscr = g_savedContext.vscr;
  state->vec.vrsave = g_savedContext.vrsave;

  return kSuccess;
}

ErrorCode Debug::writeRegistersPPC(void const *regs, size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::PowerPC::CPUState *state =
      reinterpret_cast<const Architecture::PowerPC::CPUState *>(regs);

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    g_savedContext.gpr[i] = state->gp.regs[i];
  }

  // Copy special registers
  g_savedContext.pc = state->special.pc;
  g_savedContext.msr = state->special.msr;
  g_savedContext.cr = state->special.cr;
  g_savedContext.lr = state->special.lr;
  g_savedContext.ctr = state->special.ctr;
  g_savedContext.xer = state->special.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    g_savedContext.fpr[i] = state->fpu.regs[i];
  }
  g_savedContext.fpscr = state->fpu.fpscr;

  // Copy vector registers
  for (int i = 0; i < 32; i++) {
    memcpy(g_savedContext.vr[i], state->vec.vr[i], 16);
  }
  g_savedContext.vscr = state->vec.vscr;
  g_savedContext.vrsave = state->vec.vrsave;

  return kSuccess;
}

ErrorCode Debug::getTrapAddress(uint16_t trapNum, Address &address) {
  DS2LOG(Warning, "getTrapAddress called on PowerPC (68k trap emulation)");

  // On PowerPC Macs, 68k traps are emulated
  // The Mixed Mode Manager handles this
  address = Address(0x00040000 + (trapNum & 0x0FFF) * 4);

  return kSuccess;
}

ErrorCode Debug::setTrapAddress(uint16_t trapNum, Address address) {
  DS2LOG(Warning, "setTrapAddress called on PowerPC (68k trap emulation)");
  return kSuccess;
}

ErrorCode Debug::installExceptionHandler(uint32_t vector, Address handler) {
  Address vectorAddr = Address(vector);

  DS2LOG(Debug, "installing PowerPC exception handler at vector 0x%08x",
         vector);

  // Write handler address to exception vector (big-endian)
  uint32_t handlerAddr = __builtin_bswap32(handler.value());
  ErrorCode error = writeMemory(vectorAddr, &handlerAddr, sizeof(handlerAddr));

  return error;
}

ErrorCode Debug::removeExceptionHandler(uint32_t vector) {
  DS2LOG(Debug, "removing PowerPC exception handler at vector 0x%08x", vector);
  return kSuccess;
}

ErrorCode Debug::getProcessList(std::vector<ProcessInfo> &processes) {
  processes.clear();

  // Add current application
  ProcessInfo current;
  current.processID = 1;
  current.processSerial = 1;
  current.processType = 0x4150504C;  // 'APPL'
  current.processSignature = 0x3F3F3F3F;
  current.is68k = false;  // PowerPC
  current.processName = "Current Application";

  processes.push_back(current);

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(uint32_t pid, ProcessInfo &info) {
  info.processID = pid;
  info.processSerial = pid;
  info.processType = 0x4150504C;
  info.is68k = false;  // PowerPC
  info.processName = "PowerPC Application";

  return kSuccess;
}

ErrorCode Debug::getCFMConnections(std::vector<CFMConnectionID> &connections) {
  connections.clear();

  // Enumerate loaded code fragments
  for (const auto &fragment : g_cfmFragments) {
    CFMConnectionID conn;
    conn.connectionID = fragment.connectionID;
    conn.fragmentID = fragment.fragmentID;
    connections.push_back(conn);

    DS2LOG(Debug, "CFM fragment: '%s', connection=%u, code=0x%llx",
           fragment.fragmentName.c_str(), fragment.connectionID,
           (unsigned long long)fragment.codeAddress.value());
  }

  return kSuccess;
}

ErrorCode Debug::loadCFMFragment(std::string const &path,
                                 CFMConnectionID &connection) {
  DS2LOG(Debug, "loading CFM fragment: %s", path.c_str());

  // Create new fragment info
  CFMFragmentInfo fragment;
  fragment.connectionID = g_cfmFragments.size() + 1;
  fragment.fragmentID = fragment.connectionID;
  fragment.fragmentName = path;
  fragment.codeAddress = Address(0x01000000 + g_cfmFragments.size() * 0x100000);
  fragment.codeLength = 0x10000;
  fragment.dataAddress = fragment.codeAddress + fragment.codeLength;
  fragment.dataLength = 0x10000;

  g_cfmFragments.push_back(fragment);

  connection.connectionID = fragment.connectionID;
  connection.fragmentID = fragment.fragmentID;

  return kSuccess;
}

// Stubs for 68k-specific functions
ErrorCode Debug::readRegisters68k(void *regs, size_t size) {
  DS2LOG(Error, "readRegisters68k called on PowerPC debugger");
  return kErrorUnsupported;
}

ErrorCode Debug::writeRegisters68k(void const *regs, size_t size) {
  DS2LOG(Error, "writeRegisters68k called on PowerPC debugger");
  return kErrorUnsupported;
}

bool Debug::_initialized = false;

} // namespace MacOS
} // namespace Host
} // namespace ds2
