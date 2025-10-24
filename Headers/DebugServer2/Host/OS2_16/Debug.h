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
#include <vector>

namespace ds2 {
namespace Host {
namespace OS2_16 {

// OS/2 16-bit Debugging Support (OS/2 1.x)
// Uses NE (New Executable) format
// Debugging via DosDebug API

// Process and thread IDs
typedef uint16_t pid_t;
typedef uint16_t tid_t;

// Session ID
typedef uint16_t sid_t;

// DosDebug commands
enum DebugCommand {
  DBG_C_NULL = 0,             // Null command
  DBG_C_ReadMem = 1,          // Read memory
  DBG_C_ReadMem_I = 2,        // Read memory (I-space)
  DBG_C_ReadMem_D = 3,        // Read memory (D-space)
  DBG_C_WriteMem = 4,         // Write memory
  DBG_C_WriteMem_I = 5,       // Write memory (I-space)
  DBG_C_WriteMem_D = 6,       // Write memory (D-space)
  DBG_C_ReadReg = 7,          // Read registers
  DBG_C_WriteReg = 8,         // Write registers
  DBG_C_Go = 9,               // Continue execution
  DBG_C_Term = 10,            // Terminate debuggee
  DBG_C_SStep = 11,           // Single step
  DBG_C_Stop = 12,            // Stop execution
  DBG_C_Freeze = 13,          // Freeze thread
  DBG_C_Resume = 14,          // Resume thread
  DBG_C_NumToAddr = 15,       // Convert object:offset to address
  DBG_C_ReadCoRegs = 16,      // Read coprocessor registers
  DBG_C_WriteCoRegs = 17,     // Write coprocessor registers
  DBG_C_ThrdStat = 18,        // Get thread status
  DBG_C_MapROAlias = 19,      // Map read-only alias
  DBG_C_MapRWAlias = 20,      // Map read-write alias
  DBG_C_UnMapAlias = 21,      // Unmap alias
  DBG_C_Connect = 22,         // Connect to debuggee
  DBG_C_ReadMemBuf = 23,      // Read memory buffer
  DBG_C_WriteMemBuf = 24,     // Write memory buffer
  DBG_C_SetWatch = 25,        // Set watchpoint
  DBG_C_ClearWatch = 26,      // Clear watchpoint
  DBG_C_RangeStep = 27,       // Step over range
  DBG_C_LinToSel = 28,        // Linear to selector:offset
  DBG_C_SelToLin = 29,        // Selector:offset to linear
};

// Debug notification codes
enum DebugNotification {
  DBG_N_ModuleLoad = 1,       // Module loaded
  DBG_N_ModuleFree = 2,       // Module freed
  DBG_N_NewProc = 3,          // New process
  DBG_N_ProcTerm = 4,         // Process terminated
  DBG_N_ThreadCreate = 5,     // Thread created
  DBG_N_ThreadTerm = 6,       // Thread terminated
  DBG_N_Exception = 7,        // Exception occurred
  DBG_N_Breakpoint = 8,       // Breakpoint hit
  DBG_N_WatchPoint = 9,       // Watchpoint triggered
  DBG_N_SStep = 10,           // Single step complete
  DBG_N_AsyncStop = 11,       // Asynchronous stop
  DBG_N_RangeStep = 12,       // Range step complete
  DBG_N_CoError = 13,         // Coprocessor error
  DBG_N_AliasFree = 14,       // Alias freed
};

// Exception types
enum ExceptionType {
  XCPT_SIGNAL = 0x00000001,           // Signal
  XCPT_ACCESS_VIOLATION = 0xC0000005, // Access violation
  XCPT_INTEGER_DIVIDE_BY_ZERO = 0xC0000094, // Divide by zero
  XCPT_ILLEGAL_INSTRUCTION = 0xC000001C, // Illegal instruction
  XCPT_PRIVILEGED_INSTRUCTION = 0xC0000096, // Privileged instruction
  XCPT_BREAKPOINT = 0x80000003,       // INT 3
  XCPT_SINGLE_STEP = 0x80000004,      // Single step
  XCPT_FLOAT_DENORMAL_OPERAND = 0xC000008C, // Float denormal
  XCPT_FLOAT_DIVIDE_BY_ZERO = 0xC000008D, // Float divide by zero
  XCPT_FLOAT_INEXACT_RESULT = 0xC000008E, // Float inexact
  XCPT_FLOAT_INVALID_OPERATION = 0xC000008F, // Float invalid op
  XCPT_FLOAT_OVERFLOW = 0xC0000090,   // Float overflow
  XCPT_FLOAT_STACK_CHECK = 0xC0000091, // Float stack check
  XCPT_FLOAT_UNDERFLOW = 0xC0000092,  // Float underflow
  XCPT_INTEGER_OVERFLOW = 0xC0000095, // Integer overflow
  XCPT_DATATYPE_MISALIGNMENT = 0x80000002, // Misalignment
  XCPT_GUARD_PAGE_VIOLATION = 0x80000001, // Guard page
  XCPT_UNABLE_TO_GROW_STACK = 0xC00000FD, // Stack overflow
};

// Debug buffer structure
struct DebugBuffer {
  uint32_t Pid;               // Process ID
  uint32_t Tid;               // Thread ID
  uint32_t Cmd;               // Command
  uint32_t Value;             // Value/result
  uint32_t Addr;              // Address (selector:offset)
  uint32_t Buffer;            // Buffer address
  uint32_t Len;               // Length
  uint32_t Index;             // Index
  uint32_t MTE;               // Module handle
  uint32_t EAX;               // Register values
  uint32_t ECX;
  uint32_t EDX;
  uint32_t EBX;
  uint32_t ESP;
  uint32_t EBP;
  uint32_t ESI;
  uint32_t EDI;
  uint32_t EFlags;
  uint32_t EIP;
  uint32_t CSLim;             // CS limit
  uint32_t CSBase;            // CS base
  uint32_t CSAcc;             // CS access
  uint32_t CSAtr;             // CS attributes
  uint32_t CS;                // CS selector
  uint32_t DSLim;             // DS limit
  uint32_t DSBase;            // DS base
  uint32_t DSAcc;             // DS access
  uint32_t DSAtr;             // DS attributes
  uint32_t DS;                // DS selector
  uint32_t ESLim;             // ES limit
  uint32_t ESBase;            // ES base
  uint32_t ESAcc;             // ES access
  uint32_t ESAtr;             // ES attributes
  uint32_t ES;                // ES selector
  uint32_t FSLim;             // FS limit
  uint32_t FSBase;            // FS base
  uint32_t FSAcc;             // FS access
  uint32_t FSAtr;             // FS attributes
  uint32_t FS;                // FS selector
  uint32_t GSLim;             // GS limit
  uint32_t GSBase;            // GS base
  uint32_t GSAcc;             // GS access
  uint32_t GSAtr;             // GS attributes
  uint32_t GS;                // GS selector
  uint32_t SSLim;             // SS limit
  uint32_t SSBase;            // SS base
  uint32_t SSAcc;             // SS access
  uint32_t SSAtr;             // SS attributes
  uint32_t SS;                // SS selector
};

// Module load notification
struct ModuleLoadInfo {
  uint32_t hModule;           // Module handle
  uint32_t hFile;             // File handle
  uint32_t ObjectCount;       // Number of objects
  char ModuleName[256];       // Module name
};

// Thread information
struct ThreadInfo {
  uint32_t Tid;               // Thread ID
  uint32_t State;             // Thread state
  uint32_t Priority;          // Priority
  uint32_t SystemPriority;    // System priority
};

// Thread states
enum ThreadState {
  THREAD_READY = 0,           // Ready
  THREAD_BLOCKED = 1,         // Blocked
  THREAD_SUSPENDED = 2,       // Suspended
  THREAD_CRITICAL = 4,        // Critical section
  THREAD_RUNNING = 5,         // Running
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Process control
  static ErrorCode attach(pid_t pid);
  static ErrorCode detach(pid_t pid);
  static ErrorCode terminate(pid_t pid);

  // Thread control
  static ErrorCode freezeThread(pid_t pid, tid_t tid);
  static ErrorCode resumeThread(pid_t pid, tid_t tid);
  static ErrorCode getThreads(pid_t pid, std::vector<tid_t> &threads);
  static ErrorCode getThreadInfo(pid_t pid, tid_t tid, ThreadInfo &info);

  // Execution control
  static ErrorCode go(pid_t pid, tid_t tid);
  static ErrorCode singleStep(pid_t pid, tid_t tid);
  static ErrorCode stop(pid_t pid);

  // Breakpoints and watchpoints
  static ErrorCode setBreakpoint(pid_t pid, uint16_t selector, uint16_t offset);
  static ErrorCode clearBreakpoint(pid_t pid, uint16_t selector, uint16_t offset);
  static ErrorCode setWatchpoint(pid_t pid, uint32_t addr, uint32_t size, uint32_t type);
  static ErrorCode clearWatchpoint(pid_t pid, uint32_t addr);

  // Memory operations (segmented addressing)
  static ErrorCode readMemory(pid_t pid, uint16_t selector, uint16_t offset,
                             void *data, size_t size);
  static ErrorCode writeMemory(pid_t pid, uint16_t selector, uint16_t offset,
                              const void *data, size_t size);

  // Linear memory operations
  static ErrorCode readLinearMemory(pid_t pid, uint32_t addr, void *data, size_t size);
  static ErrorCode writeLinearMemory(pid_t pid, uint32_t addr, const void *data, size_t size);

  // Address conversion
  static ErrorCode selOffsetToLinear(pid_t pid, uint16_t selector, uint16_t offset,
                                    uint32_t &linear);
  static ErrorCode linearToSelOffset(pid_t pid, uint32_t linear,
                                    uint16_t &selector, uint16_t &offset);

  // Register operations
  static ErrorCode getRegisters(pid_t pid, tid_t tid, DebugBuffer &regs);
  static ErrorCode setRegisters(pid_t pid, tid_t tid, const DebugBuffer &regs);

  // Module information
  static ErrorCode getModules(pid_t pid, std::vector<ModuleLoadInfo> &modules);

  // Wait for debug event
  static ErrorCode waitForEvent(pid_t pid, DebugBuffer &event, uint32_t timeout_ms);

private:
  static bool _initialized;
};

} // namespace OS2_16
} // namespace Host
} // namespace ds2
