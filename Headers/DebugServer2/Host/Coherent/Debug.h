//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Coherent Unix - Mark Williams Company Unix clone
//
// Coherent ran on x86 PCs and provided a Unix-like environment
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace Coherent {

//
// Coherent Unix Overview
//
// Coherent was a Unix clone developed by Mark Williams Company (1983-1995):
// - Ran on x86 PCs (8086, 80286, 80386, 80486)
// - Full Unix-like environment
// - Sold as low-cost alternative to commercial Unix
// - Microkernel architecture
// - System V and BSD compatibility
// - Supported up to 2 GB of disk space (early versions: 512 MB)
// - Multi-user, multitasking
//

//
// Process States
//

enum class ProcessState {
  RUNNING,      // Process is running
  SLEEPING,     // Process is sleeping
  STOPPED,      // Process stopped by signal or debugger
  ZOMBIE,       // Process terminated but not reaped
  WAITING,      // Waiting for event
};

//
// Memory Regions
//

struct MemoryRegion {
  uint32_t start;           // Start address
  uint32_t end;             // End address
  uint32_t permissions;     // Protection bits
  char name[256];           // Region name
};

//
// Coherent Debugging Interface
//

class Debug {
public:
  //
  // Process control
  //

  static ErrorCode attach(ProcessId pid);
  static ErrorCode detach(ProcessId pid);
  static ErrorCode suspend(ProcessId pid);
  static ErrorCode resume(ProcessId pid);
  static ErrorCode terminate(ProcessId pid, int signal);

  //
  // Memory operations
  //

  static ErrorCode readMemory(ProcessId pid, uint32_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(ProcessId pid, uint32_t address,
                               const void *buffer, size_t length);

  //
  // Register operations (x86 architecture)
  //

  static ErrorCode readCPUState(ProcessId pid, Architecture::X86::CPUState &state);
  static ErrorCode writeCPUState(ProcessId pid, const Architecture::X86::CPUState &state);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint32_t address);
  static ErrorCode removeBreakpoint(ProcessId pid, uint32_t address);

  //
  // Memory mapping
  //

  static ErrorCode enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions);

  //
  // Coherent-specific features
  //

  struct ProcessInfo {
    ProcessId pid;
    ProcessId ppid;
    ProcessId pgrp;
    uint32_t uid;
    uint32_t gid;
    ProcessState state;
    char comm[16];
  };

  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);
};

//
// Coherent System Calls
//

namespace SystemCalls {
  constexpr int COH_exit       = 1;
  constexpr int COH_fork       = 2;
  constexpr int COH_read       = 3;
  constexpr int COH_write      = 4;
  constexpr int COH_open       = 5;
  constexpr int COH_close      = 6;
  constexpr int COH_wait       = 7;
  constexpr int COH_creat      = 8;
  constexpr int COH_link       = 9;
  constexpr int COH_unlink     = 10;
  constexpr int COH_exec       = 11;
  constexpr int COH_chdir      = 12;
}

//
// Coherent Signals
//

namespace Signals {
  constexpr int SIGHUP     = 1;
  constexpr int SIGINT     = 2;
  constexpr int SIGQUIT    = 3;
  constexpr int SIGILL     = 4;
  constexpr int SIGTRAP    = 5;
  constexpr int SIGIOT     = 6;
  constexpr int SIGEMT     = 7;
  constexpr int SIGFPE     = 8;
  constexpr int SIGKILL    = 9;
  constexpr int SIGBUS     = 10;
  constexpr int SIGSEGV    = 11;
  constexpr int SIGSYS     = 12;
  constexpr int SIGPIPE    = 13;
  constexpr int SIGALRM    = 14;
  constexpr int SIGTERM    = 15;
}

} // namespace Coherent
} // namespace Host
} // namespace ds2
