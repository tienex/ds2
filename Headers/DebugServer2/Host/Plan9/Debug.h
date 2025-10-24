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
namespace Plan9 {

// Plan 9 / Inferno Debugging Support
// Text-based /proc interface

// Process/thread ID
typedef int32_t pid_t;

// Process states (from /proc/n/status)
enum proc_state {
  STATE_RUNNING,      // Running
  STATE_READY,        // Ready to run
  STATE_RENDEZ,       // Rendezvous (waiting)
  STATE_QUEUEING,     // Queueing for I/O
  STATE_MORIBUND,     // Dying
  STATE_DEAD,         // Dead
  STATE_WAKEME,       // Waiting for wakeup
  STATE_BROKEN,       // Broken (debugged)
  STATE_STOPPED,      // Stopped
  STATE_WAITRELEASE,  // Waiting for release
};

// Process information from /proc/n/status
struct proc_status {
  pid_t pid;                  // Process ID
  char name[64];              // Process name
  char user[32];              // User name
  proc_state state;           // Process state
  uint64_t pc;                // Program counter
  uint64_t sp;                // Stack pointer
  uint32_t priority;          // Priority
  uint64_t runtime;           // Runtime in ticks
  uint64_t mem;               // Memory usage
};

// Memory segment from /proc/n/segment
struct proc_segment {
  char type[16];              // Segment type (Text, Data, Bss, Stack)
  uint64_t base;              // Base address
  uint64_t top;               // Top address
  uint64_t size;              // Size
  char perm[8];               // Permissions (rwx)
};

// File descriptor from /proc/n/fd
struct proc_fd {
  int fd;                     // File descriptor number
  char mode[8];               // Open mode
  char path[256];             // File path
  uint64_t offset;            // Current offset
};

// Wait message from /proc/n/wait
struct proc_wait {
  pid_t pid;                  // Process ID
  char msg[256];              // Wait message
  uint64_t time[3];           // User, sys, real time
};

// Debug control commands (to /proc/n/ctl)
#define CTL_HANG        "hang"       // Stop process
#define CTL_UNHANG      "unhang"     // Resume process
#define CTL_KILL        "kill"       // Kill process
#define CTL_STARTSTOP   "startstop"  // Stop at next instruction
#define CTL_WAITSTOP    "waitstop"   // Wait for stop
#define CTL_START       "start"      // Start execution
#define CTL_STEP        "step"       // Single step
#define CTL_CLOSE       "close"      // Close process
#define CTL_PRIVATE     "private"    // Make memory private

// Debugging operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Process control via /proc/n/ctl
  static ErrorCode attachProcess(pid_t pid);
  static ErrorCode detachProcess(pid_t pid);
  static ErrorCode killProcess(pid_t pid);
  static ErrorCode hangProcess(pid_t pid);
  static ErrorCode unhangProcess(pid_t pid);
  static ErrorCode stepProcess(pid_t pid);
  static ErrorCode startProcess(pid_t pid);
  static ErrorCode waitStop(pid_t pid);

  // Breakpoints via /proc/n/text
  static ErrorCode setBreakpoint(pid_t pid, Address address);
  static ErrorCode clearBreakpoint(pid_t pid, Address address);

  // Memory operations via /proc/n/mem
  static ErrorCode readMemory(pid_t pid, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(pid_t pid, Address address, void const *data,
                              size_t size);

  // Register operations via /proc/n/regs (text format)
  static ErrorCode readRegisters(pid_t pid, void *regs, size_t size);
  static ErrorCode writeRegisters(pid_t pid, void const *regs, size_t size);

  // Floating point via /proc/n/fpregs
  static ErrorCode readFPRegisters(pid_t pid, void *fpregs, size_t size);
  static ErrorCode writeFPRegisters(pid_t pid, void const *fpregs,
                                   size_t size);

  // Information queries
  static ErrorCode getStatus(pid_t pid, proc_status &status);
  static ErrorCode getSegments(pid_t pid, std::vector<proc_segment> &segments);
  static ErrorCode getFileDescriptors(pid_t pid, std::vector<proc_fd> &fds);
  static ErrorCode waitForProcess(pid_t pid, proc_wait &wait);

  // Process enumeration via /proc
  static ErrorCode getProcessList(std::vector<pid_t> &pids);

  // Read text-based proc files
  static ErrorCode readProcFile(pid_t pid, const char *filename,
                               std::string &content);
  static ErrorCode writeProcFile(pid_t pid, const char *filename,
                                const std::string &content);

  // Parse register file (architecture-specific text format)
  static ErrorCode parseRegisterFile(const std::string &content, void *regs,
                                    size_t size);
  static ErrorCode formatRegisterFile(void const *regs, size_t size,
                                     std::string &content);

private:
  static bool _initialized;
};

} // namespace Plan9
} // namespace Host
} // namespace ds2
