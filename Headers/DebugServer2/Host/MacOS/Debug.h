//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Types.h"

#include <cstdint>
#include <string>

namespace ds2 {
namespace Host {
namespace MacOS {

// Classic Mac OS Debugging Support
// Uses MacsBug debugging protocol and low-level debugger traps
// Supports 68k (System 1-9) and PowerPC (System 7.1.2-9.2.2)

// MacsBug Command Codes
enum MacsBugCommand {
  kMacsBugNMI = 0,           // Non-maskable interrupt
  kMacsBugBreakpoint = 1,    // Breakpoint trap
  kMacsBugTrace = 2,         // Single step
  kMacsBugIllegalInst = 3,   // Illegal instruction
  kMacsBugDebugger = 4,      // Debugger() trap
  kMacsBugDebugStr = 5,      // DebugStr() trap
};

// 68k Exception Vectors
enum M68kException {
  kM68kResetSP = 0,          // Reset: Initial SSP
  kM68kResetPC = 1,          // Reset: Initial PC
  kM68kBusError = 2,         // Bus error
  kM68kAddressError = 3,     // Address error
  kM68kIllegalInst = 4,      // Illegal instruction
  kM68kZeroDivide = 5,       // Zero divide
  kM68kCHK = 6,              // CHK instruction
  kM68kTRAPV = 7,            // TRAPV instruction
  kM68kPrivilegeViolation = 8, // Privilege violation
  kM68kTrace = 9,            // Trace
  kM68kLineA = 10,           // Line A emulator
  kM68kLineF = 11,           // Line F emulator
  kM68kTrap0 = 32,           // TRAP #0 (OS trap dispatcher)
};

// PowerPC Exception Vectors
enum PowerPCException {
  kPPCSystemReset = 0x00100,      // System reset
  kPPCMachineCheck = 0x00200,     // Machine check
  kPPCDSI = 0x00300,              // Data storage interrupt
  kPPCISI = 0x00400,              // Instruction storage interrupt
  kPPCExternal = 0x00500,         // External interrupt
  kPPCAlignment = 0x00600,        // Alignment
  kPPCProgram = 0x00700,          // Program
  kPPCFPUnavail = 0x00800,        // FP unavailable
  kPPCDecrementer = 0x00900,      // Decrementer
  kPPCSystemCall = 0x00C00,       // System call
  kPPCTrace = 0x00D00,            // Trace
};

// Toolbox Trap Numbers
enum ToolboxTrap {
  kTrapDebugger = 0xA9FF,         // Debugger()
  kTrapDebugStr = 0xABFF,         // DebugStr()
  kTrapSysError = 0xA9C9,         // SysError()
  kTrapGetTrapAddress = 0xA146,   // GetTrapAddress()
  kTrapSetTrapAddress = 0xA047,   // SetTrapAddress()
};

// Low Memory Globals (key debugging locations)
struct LowMemGlobals {
  uint32_t CurrentA5;        // 0x0904 - Current A5 (globals)
  uint32_t CurStackBase;     // 0x0908 - Current stack base
  uint32_t CurApName;        // 0x0910 - Current app name
  uint32_t CurJTOffset;      // 0x0934 - Jump table offset
  uint32_t CurrentA4;        // 0x0904 - Current A4 (QuickDraw)
  uint32_t JStash;           // 0x09EA - Jump table stash
  uint32_t BufPtr;           // 0x010C - Top of application heap
  uint32_t DefltStack;       // 0x0322 - Default stack size
  uint32_t ApplZone;         // 0x02AA - Application heap zone
  uint32_t SysZone;          // 0x02A6 - System heap zone
  uint32_t TheZone;          // 0x0118 - Current heap zone
};

// Code Fragment Manager (CFM) for PowerPC
struct CFMConnectionID {
  uint32_t connectionID;     // Connection identifier
  uint32_t fragmentID;       // Fragment identifier
};

// Classic Mac OS Process Information
struct ProcessInfo {
  uint32_t processID;        // Process serial number (high)
  uint32_t processSerial;    // Process serial number (low)
  uint32_t processType;      // Process type ('APPL', 'INIT', etc.)
  uint32_t processSignature; // Process signature (creator code)
  uint32_t processMode;      // Process mode flags
  bool is68k;                // True if 68k, false if PowerPC
  std::string processName;   // Process name (Pascal string)
};

// Debugging Operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Process control
  static ErrorCode attach(ProcessInfo const &info);
  static ErrorCode detach(ProcessInfo const &info);

  // Breakpoints
  static ErrorCode setBreakpoint(Address address);
  static ErrorCode removeBreakpoint(Address address);

  // Execution control
  static ErrorCode resume(ProcessInfo const &info);
  static ErrorCode suspend(ProcessInfo const &info);
  static ErrorCode step(ProcessInfo const &info);

  // Memory access
  static ErrorCode readMemory(Address address, void *data, size_t size);
  static ErrorCode writeMemory(Address address, void const *data, size_t size);

  // Register access (architecture-specific)
  static ErrorCode readRegisters68k(void *regs, size_t size);
  static ErrorCode writeRegisters68k(void const *regs, size_t size);
  static ErrorCode readRegistersPPC(void *regs, size_t size);
  static ErrorCode writeRegistersPPC(void const *regs, size_t size);

  // Toolbox trap handling
  static ErrorCode getTrapAddress(uint16_t trapNum, Address &address);
  static ErrorCode setTrapAddress(uint16_t trapNum, Address address);

  // Exception handling
  static ErrorCode installExceptionHandler(uint32_t vector, Address handler);
  static ErrorCode removeExceptionHandler(uint32_t vector);

  // Process information
  static ErrorCode getProcessList(std::vector<ProcessInfo> &processes);
  static ErrorCode getProcessInfo(uint32_t pid, ProcessInfo &info);

  // CFM (PowerPC only)
  static ErrorCode getCFMConnections(std::vector<CFMConnectionID> &connections);
  static ErrorCode loadCFMFragment(std::string const &path,
                                   CFMConnectionID &connection);

private:
  static bool _initialized;
};

} // namespace MacOS
} // namespace Host
} // namespace ds2
