//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Intergraph CLIX - Unix System V variant for Clipper
//
// CLIX ran on Intergraph workstations with Clipper processors
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"
#include "DebugServer2/Architecture/Clipper/CPUState.h"

namespace ds2 {
namespace Host {
namespace CLIX {

//
// Intergraph CLIX Overview
//
// CLIX was Intergraph's Unix operating system based on System V Release 3:
// - Ran on Clipper RISC processors (C100, C300, C400)
// - Used in Intergraph workstations for CAD/CAM applications
// - High-performance graphics support
// - System V IPC (shared memory, semaphores, message queues)
// - Berkeley networking extensions
// - X11 windowing system
//

//
// Process States
//

enum class ProcessState {
  RUNNING,      // Process is running
  SLEEPING,     // Process is sleeping (waiting for event)
  STOPPED,      // Process stopped by signal or debugger
  ZOMBIE,       // Process terminated but not reaped
  ONPROC,       // Process on processor (multiprocessor)
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
// CLIX Debugging Interface
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
  // Register operations (Clipper architecture)
  //

  static ErrorCode readCPUState(ProcessId pid, Architecture::Clipper::CPUState &state);
  static ErrorCode writeCPUState(ProcessId pid, const Architecture::Clipper::CPUState &state);

  // Read/write specific register sets
  static ErrorCode readGeneralRegisters(ProcessId pid, uint32_t regs[16]);
  static ErrorCode writeGeneralRegisters(ProcessId pid, const uint32_t regs[16]);

  static ErrorCode readFloatingPointRegisters(ProcessId pid, double fpregs[8]);
  static ErrorCode writeFloatingPointRegisters(ProcessId pid, const double fpregs[8]);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint32_t address);
  static ErrorCode removeBreakpoint(ProcessId pid, uint32_t address);

  // Hardware breakpoints (Clipper has limited hardware breakpoint support)
  static ErrorCode setHardwareBreakpoint(ProcessId pid, uint32_t address);
  static ErrorCode removeHardwareBreakpoint(ProcessId pid, uint32_t address);

  //
  // Watchpoints
  //

  static ErrorCode setWatchpoint(ProcessId pid, uint32_t address, size_t size, int type);
  static ErrorCode removeWatchpoint(ProcessId pid, uint32_t address);

  //
  // Memory mapping
  //

  static ErrorCode enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions);

  //
  // CLIX specific features
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
    uint32_t priority;      // Scheduling priority
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

  // System V IPC information
  struct IPCInfo {
    uint32_t num_shm_segments;  // Number of shared memory segments
    uint32_t num_semaphores;    // Number of semaphore sets
    uint32_t num_msg_queues;    // Number of message queues
  };

  static ErrorCode getIPCInfo(ProcessId pid, IPCInfo &info);
};

//
// CLIX System Calls (System V Release 3 based)
//

namespace SystemCalls {
  constexpr int CLIX_exit      = 1;
  constexpr int CLIX_fork      = 2;
  constexpr int CLIX_read      = 3;
  constexpr int CLIX_write     = 4;
  constexpr int CLIX_open      = 5;
  constexpr int CLIX_close     = 6;
  constexpr int CLIX_wait      = 7;
  constexpr int CLIX_creat     = 8;
  constexpr int CLIX_link      = 9;
  constexpr int CLIX_unlink    = 10;
  constexpr int CLIX_exec      = 11;
  constexpr int CLIX_chdir     = 12;

  // System V IPC
  constexpr int CLIX_msgget    = 49;
  constexpr int CLIX_msgctl    = 50;
  constexpr int CLIX_msgrcv    = 51;
  constexpr int CLIX_msgsnd    = 52;
  constexpr int CLIX_shmget    = 52;
  constexpr int CLIX_shmctl    = 53;
  constexpr int CLIX_shmat     = 54;
  constexpr int CLIX_shmdt     = 55;
  constexpr int CLIX_semget    = 56;
  constexpr int CLIX_semctl    = 57;
  constexpr int CLIX_semop     = 58;

  // File locking
  constexpr int CLIX_fcntl     = 62;

  // Process control
  constexpr int CLIX_setpgrp   = 39;
  constexpr int CLIX_getpgrp   = 40;

  // Signals
  constexpr int CLIX_signal    = 48;
  constexpr int CLIX_kill      = 37;

  // Polling
  constexpr int CLIX_poll      = 87;
}

//
// CLIX Signals (System V)
//

namespace Signals {
  constexpr int SIGHUP     = 1;   // Hangup
  constexpr int SIGINT     = 2;   // Interrupt
  constexpr int SIGQUIT    = 3;   // Quit
  constexpr int SIGILL     = 4;   // Illegal instruction
  constexpr int SIGTRAP    = 5;   // Trace trap
  constexpr int SIGIOT     = 6;   // IOT instruction (same as SIGABRT)
  constexpr int SIGABRT    = 6;   // Abort
  constexpr int SIGEMT     = 7;   // EMT instruction
  constexpr int SIGFPE     = 8;   // Floating point exception
  constexpr int SIGKILL    = 9;   // Kill (cannot be caught or ignored)
  constexpr int SIGBUS     = 10;  // Bus error
  constexpr int SIGSEGV    = 11;  // Segmentation violation
  constexpr int SIGSYS     = 12;  // Bad system call
  constexpr int SIGPIPE    = 13;  // Broken pipe
  constexpr int SIGALRM    = 14;  // Alarm clock
  constexpr int SIGTERM    = 15;  // Software termination
  constexpr int SIGUSR1    = 16;  // User defined signal 1
  constexpr int SIGUSR2    = 17;  // User defined signal 2
  constexpr int SIGCHLD    = 18;  // Child status changed
  constexpr int SIGPWR     = 19;  // Power fail/restart
  constexpr int SIGWINCH   = 20;  // Window size change
  constexpr int SIGURG     = 21;  // Urgent condition on socket
  constexpr int SIGPOLL    = 22;  // Pollable event (SIGIO)
  constexpr int SIGIO      = 22;  // I/O possible
  constexpr int SIGSTOP    = 23;  // Stop (cannot be caught or ignored)
  constexpr int SIGTSTP    = 24;  // Stop signal from tty
  constexpr int SIGCONT    = 25;  // Continue after stop
  constexpr int SIGTTIN    = 26;  // Background read from tty
  constexpr int SIGTTOU    = 27;  // Background write to tty
  constexpr int SIGVTALRM  = 28;  // Virtual timer expired
  constexpr int SIGPROF    = 29;  // Profiling timer expired
  constexpr int SIGXCPU    = 30;  // CPU time limit exceeded
  constexpr int SIGXFSZ    = 31;  // File size limit exceeded
}

//
// ptrace requests (CLIX/System V)
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

} // namespace CLIX
} // namespace Host
} // namespace ds2
