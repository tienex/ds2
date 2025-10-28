//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// 4.3BSD-Tahoe - BSD Unix variant
//
// 4.3BSD-Tahoe ran on:
// - CCI Power 6/32 (Tahoe architecture - VAX-like CISC)
// - Harris HCX-9 (also used Tahoe architecture)
// - Other Tahoe-based systems
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace BSD_Tahoe {

//
// 4.3BSD-Tahoe Overview
//
// 4.3BSD-Tahoe (1988) was an important BSD release that:
// - Removed VAX-specific code to support multiple architectures
// - Introduced the first multi-architecture BSD
// - Named after the CCI Power 6/32 "Tahoe" architecture
// - Introduced many networking improvements
// - Added NFS support
// - Basis for many later BSD variants
//

//
// Process States
//

enum class ProcessState {
  RUNNING,      // Process is running
  SLEEPING,     // Process is sleeping (waiting for event)
  STOPPED,      // Process stopped by signal or debugger
  ZOMBIE,       // Process terminated but not reaped
  IDLE,         // Process idle (waiting for resources)
};

//
// Memory Regions
//

struct MemoryRegion {
  uint32_t start;           // Start address
  uint32_t end;             // End address
  uint32_t permissions;     // Protection bits (PROT_READ, PROT_WRITE, PROT_EXEC)
  char name[256];           // Region name
};

//
// 4.3BSD-Tahoe Debugging Interface
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
  // Register operations (Tahoe architecture)
  //

  // Tahoe (CCI Power 6/32) CPU state - used by Harris HCX-9
  static ErrorCode readCPUState(ProcessId pid, Architecture::Tahoe::CPUState &state);
  static ErrorCode writeCPUState(ProcessId pid, const Architecture::Tahoe::CPUState &state);

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
  // 4.3BSD-Tahoe specific features
  //

  // Process information
  struct ProcessInfo {
    ProcessId pid;
    ProcessId ppid;         // Parent PID
    ProcessId pgrp;         // Process group
    uint32_t uid;           // User ID
    uint32_t gid;           // Group ID
    ProcessState state;     // Process state
    uint32_t flags;         // Process flags
    int32_t nice;           // Nice value
    char comm[16];          // Command name
  };

  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);

  // Virtual memory statistics
  struct VMStats {
    uint32_t text_size;     // Text segment size
    uint32_t data_size;     // Data segment size
    uint32_t stack_size;    // Stack segment size
    uint32_t resident_size; // Resident set size
    uint32_t shared_size;   // Shared memory size
  };

  static ErrorCode getVMStats(ProcessId pid, VMStats &stats);
};

//
// 4.3BSD-Tahoe System Calls
//

namespace SystemCalls {
  constexpr int BSD_exit       = 1;
  constexpr int BSD_fork       = 2;
  constexpr int BSD_read       = 3;
  constexpr int BSD_write      = 4;
  constexpr int BSD_open       = 5;
  constexpr int BSD_close      = 6;
  constexpr int BSD_wait       = 7;
  constexpr int BSD_creat      = 8;
  constexpr int BSD_link       = 9;
  constexpr int BSD_unlink     = 10;
  constexpr int BSD_exec       = 11;
  constexpr int BSD_chdir      = 12;

  // BSD-specific
  constexpr int BSD_vfork      = 66;
  constexpr int BSD_vhangup    = 76;
  constexpr int BSD_getgroups  = 79;
  constexpr int BSD_setgroups  = 80;
  constexpr int BSD_getpgrp    = 81;
  constexpr int BSD_setpgrp    = 82;
  constexpr int BSD_setitimer  = 83;
  constexpr int BSD_getitimer  = 86;
  constexpr int BSD_getdtablesize = 89;
  constexpr int BSD_dup2       = 90;
  constexpr int BSD_select     = 93;
}

//
// 4.3BSD-Tahoe Signals
//

namespace Signals {
  constexpr int SIGHUP     = 1;   // Hangup
  constexpr int SIGINT     = 2;   // Interrupt
  constexpr int SIGQUIT    = 3;   // Quit
  constexpr int SIGILL     = 4;   // Illegal instruction
  constexpr int SIGTRAP    = 5;   // Trace trap
  constexpr int SIGIOT     = 6;   // IOT instruction
  constexpr int SIGEMT     = 7;   // EMT instruction
  constexpr int SIGFPE     = 8;   // Floating point exception
  constexpr int SIGKILL    = 9;   // Kill
  constexpr int SIGBUS     = 10;  // Bus error
  constexpr int SIGSEGV    = 11;  // Segmentation violation
  constexpr int SIGSYS     = 12;  // Bad system call
  constexpr int SIGPIPE    = 13;  // Broken pipe
  constexpr int SIGALRM    = 14;  // Alarm clock
  constexpr int SIGTERM    = 15;  // Software termination
  constexpr int SIGURG     = 16;  // Urgent condition on socket
  constexpr int SIGSTOP    = 17;  // Stop (cannot be caught or ignored)
  constexpr int SIGTSTP    = 18;  // Stop signal from tty
  constexpr int SIGCONT    = 19;  // Continue after stop
  constexpr int SIGCHLD    = 20;  // Child status changed
  constexpr int SIGTTIN    = 21;  // Background read from tty
  constexpr int SIGTTOU    = 22;  // Background write to tty
  constexpr int SIGIO      = 23;  // I/O possible
  constexpr int SIGXCPU    = 24;  // CPU time limit exceeded
  constexpr int SIGXFSZ    = 25;  // File size limit exceeded
  constexpr int SIGVTALRM  = 26;  // Virtual timer expired
  constexpr int SIGPROF    = 27;  // Profiling timer expired
  constexpr int SIGWINCH   = 28;  // Window size change
  constexpr int SIGUSR1    = 30;  // User defined signal 1
  constexpr int SIGUSR2    = 31;  // User defined signal 2
}

//
// ptrace requests (4.3BSD-Tahoe specific)
//

#ifndef PTRACE_TRACEME
#define PTRACE_TRACEME    0   // Child declares it's being traced
#define PTRACE_PEEKTEXT   1   // Read word from text segment
#define PTRACE_PEEKDATA   2   // Read word from data segment
#define PTRACE_PEEKUSER   3   // Read word from user struct
#define PTRACE_POKETEXT   4   // Write word to text segment
#define PTRACE_POKEDATA   5   // Write word to data segment
#define PTRACE_POKEUSER   6   // Write word to user struct
#define PTRACE_CONT       7   // Continue execution
#define PTRACE_KILL       8   // Terminate process
#define PTRACE_SINGLESTEP 9   // Single step execution
#define PTRACE_ATTACH     10  // Attach to running process (4.3BSD extension)
#define PTRACE_DETACH     11  // Detach from process (4.3BSD extension)
#endif

} // namespace BSD_Tahoe
} // namespace Host
} // namespace ds2
