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
namespace Ultrix {

// DEC Ultrix Unix Debugging Support
// Ultrix-11 (PDP-11), Ultrix-32 (VAX), Ultrix (MIPS)
// DEC's Unix variant (1984-1998)

// Process and thread IDs
typedef int32_t pid_t;
typedef int32_t tid_t;

// ptrace request codes (Ultrix)
enum TraceRequest {
  PT_TRACE_ME = 0,          // Allow parent to trace
  PT_READ_I = 1,            // Read instruction space
  PT_READ_D = 2,            // Read data space
  PT_READ_U = 3,            // Read user area
  PT_WRITE_I = 4,           // Write instruction space
  PT_WRITE_D = 5,           // Write data space
  PT_WRITE_U = 6,           // Write user area
  PT_CONTINUE = 7,          // Continue execution
  PT_KILL = 8,              // Terminate process
  PT_STEP = 9,              // Single step
  PT_ATTACH = 10,           // Attach to process
  PT_DETACH = 11,           // Detach from process
  PT_GETREGS = 12,          // Get all registers
  PT_SETREGS = 13,          // Set all registers
  PT_GETFPREGS = 14,        // Get FP registers
  PT_SETFPREGS = 15,        // Set FP registers
};

// Signal numbers (Ultrix)
enum Signals {
  SIGHUP = 1,
  SIGINT = 2,
  SIGQUIT = 3,
  SIGILL = 4,
  SIGTRAP = 5,
  SIGIOT = 6,
  SIGEMT = 7,
  SIGFPE = 8,
  SIGKILL = 9,
  SIGBUS = 10,
  SIGSEGV = 11,
  SIGSYS = 12,
  SIGPIPE = 13,
  SIGALRM = 14,
  SIGTERM = 15,
  SIGURG = 16,
  SIGSTOP = 17,
  SIGTSTP = 18,
  SIGCONT = 19,
  SIGCHLD = 20,
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
  static ErrorCode wait(pid_t pid, int *status);
  static ErrorCode kill(pid_t pid, int signal);

  // Execution control
  static ErrorCode cont(pid_t pid, int signal = 0);
  static ErrorCode singleStep(pid_t pid, int signal = 0);

  // Breakpoint support
  static ErrorCode setBreakpoint(pid_t pid, Address address);
  static ErrorCode clearBreakpoint(pid_t pid, Address address);

  // Memory operations
  static ErrorCode readMemory(pid_t pid, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(pid_t pid, Address address, void const *data,
                              size_t size);

  // Register operations (architecture-specific)
  static ErrorCode getRegisters(pid_t pid, void *registers, size_t size);
  static ErrorCode setRegisters(pid_t pid, void const *registers, size_t size);
  static ErrorCode getFPRegisters(pid_t pid, void *fpregs, size_t size);
  static ErrorCode setFPRegisters(pid_t pid, void const *fpregs, size_t size);

  // Process information
  static ErrorCode getProcessInfo(pid_t pid, ProcessInfo &info);

private:
  static bool _initialized;
};

} // namespace Ultrix
} // namespace Host
} // namespace ds2
