//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// MachTen - Unix environment for classic Mac OS
//
// Developed by Tenon Intersystems, based on Mach microkernel and BSD
// Ran on 68k and PowerPC Macintosh systems (System 7, Mac OS 8/9)
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace MachTen {

//
// MachTen Overview
//
// MachTen was a Unix environment for classic Mac OS:
// - Based on Mach 2.5/2.6 microkernel with 4.3BSD/4.4BSD
// - Ran alongside Mac OS (System 7, Mac OS 8, Mac OS 9)
// - Supported 68k and PowerPC Macintosh computers
// - Full TCP/IP networking stack
// - X11 windowing system
// - NFS client and server
// - POSIX compliance
//
// Versions:
// - MachTen 2.x (1991-1994): 4.3BSD-based, 68k and PowerPC
// - MachTen 4.x (1995-1999): 4.4BSD-based, improved performance
// - MachTen Professional: Enhanced with additional features
//

//
// MachTen Versions
//

enum class Version {
  MACHTEN_2_0,              // 4.3BSD-based, 68k
  MACHTEN_2_1,              // 4.3BSD-based, 68k and PowerPC
  MACHTEN_2_2,              // 4.3BSD-based, enhanced
  MACHTEN_4_0,              // 4.4BSD-based
  MACHTEN_4_1,              // 4.4BSD-based, improved
  MACHTEN_4_1_1,            // Bug fixes
  MACHTEN_4_1_2,            // Final version
  MACHTEN_PRO,              // Professional edition
};

//
// Process States
//

enum class ProcessState {
  RUNNING,      // Process is running
  RUNNABLE,     // Process is runnable
  SLEEPING,     // Process is sleeping
  STOPPED,      // Process stopped by signal or debugger
  ZOMBIE,       // Process terminated but not reaped
  SWAPPED,      // Process swapped out
};

//
// Memory Regions
//

struct MemoryRegion {
  uint32_t start;           // Start address
  uint32_t end;             // End address
  uint32_t permissions;     // Protection bits (PROT_READ, PROT_WRITE, PROT_EXEC)
  uint32_t flags;           // MAP_PRIVATE, MAP_SHARED, etc.
  char name[256];           // Region name
};

//
// Mach Port Information
//

struct MachPort {
  uint32_t port_id;         // Mach port ID
  uint32_t port_rights;     // Port rights (send, receive, etc.)
  uint32_t queue_length;    // Message queue length
  char name[128];           // Port name
};

//
// MachTen Debugging Interface
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
  static ErrorCode singleStep(ProcessId pid);
  static ErrorCode terminate(ProcessId pid, int signal);

  //
  // Memory operations
  //

  static ErrorCode readMemory(ProcessId pid, uint32_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(ProcessId pid, uint32_t address,
                               const void *buffer, size_t length);

  //
  // Register operations (architecture-specific)
  //

  // For 68k Macs
  static ErrorCode readM68kCPUState(ProcessId pid, Architecture::M68k::CPUState &state);
  static ErrorCode writeM68kCPUState(ProcessId pid, const Architecture::M68k::CPUState &state);

  // For PowerPC Macs
  static ErrorCode readPowerPCCPUState(ProcessId pid, Architecture::PowerPC::CPUState &state);
  static ErrorCode writePowerPCCPUState(ProcessId pid, const Architecture::PowerPC::CPUState &state);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint32_t address);
  static ErrorCode removeBreakpoint(ProcessId pid, uint32_t address);

  //
  // Mach-specific operations
  //

  // Mach ports
  static ErrorCode enumeratePorts(ProcessId pid, std::vector<MachPort> &ports);
  static ErrorCode getPortInfo(ProcessId pid, uint32_t port_id, MachPort &port);

  // Mach messages
  static ErrorCode sendMachMessage(ProcessId pid, uint32_t port_id, const void *msg, size_t size);
  static ErrorCode receiveMachMessage(ProcessId pid, uint32_t port_id, void *msg, size_t size);

  //
  // Memory mapping
  //

  static ErrorCode enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions);

  //
  // MachTen-specific features
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
    uint32_t mach_task;     // Mach task port
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

  // System version
  static ErrorCode getVersion(Version &version);
};

//
// MachTen System Calls (BSD-based)
//

namespace SystemCalls {
  // Standard BSD
  constexpr int MT_exit        = 1;
  constexpr int MT_fork        = 2;
  constexpr int MT_read        = 3;
  constexpr int MT_write       = 4;
  constexpr int MT_open        = 5;
  constexpr int MT_close       = 6;
  constexpr int MT_wait        = 7;
  constexpr int MT_creat       = 8;
  constexpr int MT_link        = 9;
  constexpr int MT_unlink      = 10;
  constexpr int MT_execv       = 11;
  constexpr int MT_chdir       = 12;

  // BSD-specific
  constexpr int MT_vfork       = 66;
  constexpr int MT_select      = 93;
  constexpr int MT_getdtablesize = 89;

  // Mach-specific
  constexpr int MT_task_self   = 150;  // Get task port
  constexpr int MT_thread_self = 151;  // Get thread port
  constexpr int MT_msg_send    = 152;  // Send Mach message
  constexpr int MT_msg_receive = 153;  // Receive Mach message
  constexpr int MT_port_allocate = 154; // Allocate port
}

//
// MachTen Signals (BSD-based)
//

namespace Signals {
  constexpr int SIGHUP     = 1;   // Hangup
  constexpr int SIGINT     = 2;   // Interrupt
  constexpr int SIGQUIT    = 3;   // Quit
  constexpr int SIGILL     = 4;   // Illegal instruction
  constexpr int SIGTRAP    = 5;   // Trace trap
  constexpr int SIGABRT    = 6;   // Abort
  constexpr int SIGIOT     = 6;   // IOT instruction (same as SIGABRT)
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
// ptrace requests (BSD-based)
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
#define PTRACE_ATTACH     10  // Attach to running process
#define PTRACE_DETACH     11  // Detach from process
#endif

} // namespace MachTen
} // namespace Host
} // namespace ds2
