//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UNICOS - Cray's Unix Operating System
//
// UNICOS ran on Cray vector supercomputers (Cray-1, X-MP, Y-MP, C90, T90, SV1)
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace UNICOS {

//
// UNICOS Overview
//
// UNICOS is Cray's proprietary Unix implementation for vector supercomputers:
// - Based on System V Unix with BSD enhancements
// - Optimized for vector processing and parallel computing
// - Job control and batch processing (NQS - Network Queueing System)
// - Large memory support (up to 256 GB on later systems)
// - Custom file systems (CFS, FFIO)
//

//
// UNICOS Process Control
//

// Process states in UNICOS
enum class ProcessState {
  RUNNING,      // Process is running
  RUNNABLE,     // Process is ready to run
  SLEEPING,     // Process is sleeping (waiting for resource)
  STOPPED,      // Process is stopped (by debugger or signal)
  ZOMBIE,       // Process has terminated but not reaped
  SWAPPED,      // Process swapped out to disk
};

// UNICOS job control
struct JobInfo {
  uint64_t job_id;          // UNICOS job ID
  uint64_t user_id;         // User ID
  uint32_t priority;        // Job priority (0-255)
  uint64_t cpu_time;        // CPU time used (in clock ticks)
  uint64_t memory_limit;    // Memory limit in words (64-bit words)
  uint64_t cpu_limit;       // CPU time limit in seconds
  char job_name[256];       // Job name
  char queue_name[64];      // Queue name (for batch jobs)
  bool is_batch;            // True if batch job, false if interactive
};

//
// UNICOS Memory Management
//

// Memory region types
enum class MemoryRegionType {
  TEXT,         // Program code (read-only, shared)
  DATA,         // Initialized data
  BSS,          // Uninitialized data
  HEAP,         // Dynamic heap
  STACK,        // Stack (grows down)
  SHARED,       // Shared memory segment
  MMAPPED,      // Memory-mapped file or device
};

struct MemoryRegion {
  uint64_t start_word;      // Start address (word address, not byte)
  uint64_t end_word;        // End address (word address)
  MemoryRegionType type;    // Region type
  uint32_t permissions;     // Permission bits (read, write, execute)
  char name[256];           // Region name or file mapping
};

//
// UNICOS File System (CFS/FFIO)
//

// CFS (Cray File System) attributes
struct CFSAttributes {
  uint64_t record_size;     // Record size in words (for blocked files)
  uint64_t block_size;      // Block size in words
  uint32_t file_structure;  // File structure type
  bool is_blocked;          // Blocked vs unblocked
  bool is_binary;           // Binary vs text
  char dataset_name[256];   // Dataset name
};

// FFIO (Flexible File I/O) layer information
enum class FFIOLayer {
  SYSTEM,       // Raw system layer
  CACHE,        // Cache layer
  BUFFER,       // Buffering layer
  CONVERT,      // Data conversion layer
  BLOCKED,      // Blocked I/O layer
  TEXT,         // Text processing layer
  USER,         // User-defined layer
};

//
// UNICOS Debugging Interface
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

  // Read memory (word-addressed on early Crays)
  static ErrorCode readMemoryWords(ProcessId pid, uint64_t word_address,
                                   uint64_t *buffer, size_t word_count);

  // Write memory (word-addressed)
  static ErrorCode writeMemoryWords(ProcessId pid, uint64_t word_address,
                                    const uint64_t *buffer, size_t word_count);

  // Byte-addressed operations (for later UNICOS versions with byte addressing)
  static ErrorCode readMemory(ProcessId pid, uint64_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(ProcessId pid, uint64_t address,
                               const void *buffer, size_t length);

  //
  // Register operations
  //

  static ErrorCode readCPUState(ProcessId pid, Architecture::Cray::CPUState &state);
  static ErrorCode writeCPUState(ProcessId pid, const Architecture::Cray::CPUState &state);

  // Read specific register sets
  static ErrorCode readVectorRegisters(ProcessId pid, uint64_t vr[8][128]);
  static ErrorCode writeVectorRegisters(ProcessId pid, const uint64_t vr[8][128]);

  static ErrorCode readScalarRegisters(ProcessId pid, uint64_t s[8]);
  static ErrorCode writeScalarRegisters(ProcessId pid, const uint64_t s[8]);

  static ErrorCode readAddressRegisters(ProcessId pid, uint32_t a[8]);
  static ErrorCode writeAddressRegisters(ProcessId pid, const uint32_t a[8]);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint64_t word_address);
  static ErrorCode removeBreakpoint(ProcessId pid, uint64_t word_address);

  // Vector instruction breakpoints (break when VL changes, etc.)
  static ErrorCode setVectorLengthWatch(ProcessId pid, uint32_t vl_min, uint32_t vl_max);

  //
  // Job control
  //

  static ErrorCode getJobInfo(ProcessId pid, JobInfo &info);
  static ErrorCode setJobPriority(ProcessId pid, uint32_t priority);
  static ErrorCode getJobLimits(ProcessId pid, uint64_t &cpu_limit, uint64_t &memory_limit);

  //
  // Memory mapping
  //

  static ErrorCode enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions);
  static ErrorCode getMemoryRegionInfo(ProcessId pid, uint64_t word_address, MemoryRegion &region);

  //
  // Performance monitoring
  //

  struct PerformanceCounters {
    uint64_t cpu_cycles;          // CPU cycles
    uint64_t vector_instructions; // Vector instructions executed
    uint64_t scalar_instructions; // Scalar instructions executed
    uint64_t memory_references;   // Memory references
    uint64_t vector_operations;   // Total vector operations (sum across elements)
    uint64_t functional_unit_conflicts; // Pipeline conflicts
    double vector_utilization;    // Percentage of time in vector mode
    double mflops;                // Millions of floating-point ops per second
  };

  static ErrorCode getPerformanceCounters(ProcessId pid, PerformanceCounters &counters);

  //
  // UNICOS-specific features
  //

  // Exchange package (context switching)
  static ErrorCode getExchangePackage(ProcessId pid,
                                      Architecture::Cray::CPUState::ExchangePackage &exchange);

  // File system information
  static ErrorCode getCFSAttributes(const char *filename, CFSAttributes &attrs);

  // Get processor information
  static ErrorCode getProcessorInfo(uint32_t &num_cpus, Architecture::Cray::Variant &variant);
};

//
// UNICOS System Calls (subset)
//

namespace SystemCalls {
  constexpr int UNICOS_exit        = 1;
  constexpr int UNICOS_fork        = 2;
  constexpr int UNICOS_read        = 3;
  constexpr int UNICOS_write       = 4;
  constexpr int UNICOS_open        = 5;
  constexpr int UNICOS_close       = 6;
  constexpr int UNICOS_wait        = 7;
  constexpr int UNICOS_creat       = 8;
  constexpr int UNICOS_link        = 9;
  constexpr int UNICOS_unlink      = 10;
  constexpr int UNICOS_exec        = 11;
  constexpr int UNICOS_chdir       = 12;

  // UNICOS-specific system calls
  constexpr int UNICOS_jobctl      = 100;  // Job control
  constexpr int UNICOS_setlimit    = 101;  // Set resource limits
  constexpr int UNICOS_getlimit    = 102;  // Get resource limits
  constexpr int UNICOS_ffio        = 110;  // FFIO operations
  constexpr int UNICOS_cfs         = 111;  // CFS operations
  constexpr int UNICOS_vectorctl   = 120;  // Vector operation control
}

//
// UNICOS Signals
//

namespace Signals {
  constexpr int SIGHUP     = 1;   // Hangup
  constexpr int SIGINT     = 2;   // Interrupt
  constexpr int SIGQUIT    = 3;   // Quit
  constexpr int SIGILL     = 4;   // Illegal instruction
  constexpr int SIGTRAP    = 5;   // Trace/breakpoint trap
  constexpr int SIGIOT     = 6;   // IOT instruction
  constexpr int SIGEMT     = 7;   // EMT instruction
  constexpr int SIGFPE     = 8;   // Floating point exception
  constexpr int SIGKILL    = 9;   // Kill
  constexpr int SIGBUS     = 10;  // Bus error
  constexpr int SIGSEGV    = 11;  // Segmentation violation
  constexpr int SIGSYS     = 12;  // Bad system call
  constexpr int SIGPIPE    = 13;  // Broken pipe
  constexpr int SIGALRM    = 14;  // Alarm clock
  constexpr int SIGTERM    = 15;  // Terminate
  constexpr int SIGUSR1    = 16;  // User signal 1
  constexpr int SIGUSR2    = 17;  // User signal 2
  constexpr int SIGCHLD    = 18;  // Child status change
  constexpr int SIGPWR     = 19;  // Power fail
  constexpr int SIGVTALRM  = 20;  // Virtual timer alarm
  constexpr int SIGPROF    = 21;  // Profiling timer alarm
  constexpr int SIGIO      = 22;  // I/O possible
  constexpr int SIGWINCH   = 23;  // Window size change
  constexpr int SIGSTOP    = 24;  // Stop
  constexpr int SIGTSTP    = 25;  // Terminal stop
  constexpr int SIGCONT    = 26;  // Continue
  constexpr int SIGTTIN    = 27;  // Background read
  constexpr int SIGTTOU    = 28;  // Background write
  constexpr int SIGURG     = 29;  // Urgent condition
  constexpr int SIGLOST    = 30;  // Resource lost
  constexpr int SIGXCPU    = 31;  // CPU time limit exceeded
  constexpr int SIGXFSZ    = 32;  // File size limit exceeded
}

} // namespace UNICOS
} // namespace Host
} // namespace ds2
