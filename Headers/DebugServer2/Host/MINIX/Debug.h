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
namespace MINIX {

// MINIX Operating System Debugging Support
// MINIX 1.x, 2.x: 16-bit and 32-bit variants
// MINIX 3.x: Modern microkernel design

// Process and thread IDs
typedef int32_t pid_t;
typedef int32_t tid_t;

// MINIX version detection
enum MinixVersion {
  MINIX_V1 = 1,     // MINIX 1.x (16-bit)
  MINIX_V2 = 2,     // MINIX 2.x (32-bit)
  MINIX_V3 = 3,     // MINIX 3.x (microkernel)
};

// Debug trace request codes (MINIX ptrace)
enum TraceRequest {
  T_OK = 0,              // Not a request, just a status
  T_GETINS = 1,          // Get instruction from text
  T_GETDATA = 2,         // Get data
  T_GETUSER = 3,         // Get user area
  T_SETINS = 4,          // Set instruction in text
  T_SETDATA = 5,         // Set data
  T_SETUSER = 6,         // Set user area
  T_RESUME = 7,          // Resume execution
  T_EXIT = 8,            // Exit
  T_STEP = 9,            // Single step
  T_SYSCALL = 10,        // Trace system calls
  T_ATTACH = 11,         // Attach to process
  T_DETACH = 12,         // Detach from process
  T_SETOPT = 13,         // Set trace options
  T_GETOPT = 14,         // Get trace options
};

// Signals
enum Signals {
  SIGHUP = 1,
  SIGINT = 2,
  SIGQUIT = 3,
  SIGILL = 4,
  SIGTRAP = 5,
  SIGABRT = 6,
  SIGBUS = 7,
  SIGFPE = 8,
  SIGKILL = 9,
  SIGUSR1 = 10,
  SIGSEGV = 11,
  SIGUSR2 = 12,
  SIGPIPE = 13,
  SIGALRM = 14,
  SIGTERM = 15,
  SIGCHLD = 17,
  SIGCONT = 18,
  SIGSTOP = 19,
};

// Process states (MINIX-specific)
enum ProcessState {
  RUNNABLE = 0,         // Ready to run
  SENDING = 1,          // Blocked sending message
  RECEIVING = 2,        // Blocked receiving message
  WAITING = 3,          // Waiting for child
  STOPPED = 4,          // Stopped (debugger)
  ZOMBIE = 5,           // Zombie process
};

// Message passing (MINIX IPC)
struct Message {
  int32_t m_source;     // Sender process
  int32_t m_type;       // Message type
  union {
    uint8_t m_data[56]; // Message payload
    struct {
      int32_t m1_i1, m1_i2, m1_i3;
      char *m1_p1, *m1_p2, *m1_p3;
    } m1;
    struct {
      int32_t m2_i1, m2_i2, m2_i3;
      long m2_l1, m2_l2;
      char *m2_p1;
    } m2;
  };
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Version detection
  static ErrorCode getMinixVersion(MinixVersion &version);

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

  // Register operations
  static ErrorCode getRegisters(pid_t pid, void *registers, size_t size);
  static ErrorCode setRegisters(pid_t pid, void const *registers, size_t size);

  // Process information
  static ErrorCode getProcessInfo(pid_t pid, ProcessInfo &info);
  static ErrorCode getProcessState(pid_t pid, ProcessState &state);

  // MINIX-specific: Message passing inspection (for MINIX 3)
  static ErrorCode getMessage(pid_t pid, Message &message);

private:
  static bool _initialized;
  static MinixVersion _version;
};

} // namespace MINIX
} // namespace Host
} // namespace ds2
