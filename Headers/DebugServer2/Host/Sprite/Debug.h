//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Sprite - Distributed Operating System
//
// Developed at UC Berkeley by John Ousterhout's research group (1984-1992)
// Ran on Sun-3 (68k), DECstation (MIPS), and SPUR workstations
//

#pragma once

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace Sprite {

//
// Sprite Operating System Overview
//
// Sprite was a distributed operating system developed at UC Berkeley:
// - Network-transparent file system with client-side caching
// - Process migration for load balancing
// - Large file caching (pioneered file caching techniques)
// - Log-structured file system (LFS) research
// - Prefix tables for distributed naming
// - Virtual memory with remote paging
// - Ran on Sun-3 (m68k), DECstation (MIPS), and SPUR workstations
//
// Key innovations:
// - Log-structured file system (VLSI Layout Tool influenced later LFS)
// - Process migration across network
// - Network transparency
// - Large-scale file caching
//

//
// Process States
//

enum class ProcessState {
  NEW,          // Process just created
  READY,        // Process ready to run
  RUNNING,      // Process is running
  WAITING,      // Process waiting for event
  SUSPENDED,    // Process suspended
  EXITING,      // Process exiting
  DEAD,         // Process terminated
  MIGRATED,     // Process migrated to another host
  MIGRATING,    // Process being migrated
};

//
// Process Migration States
//

enum class MigrationState {
  NOT_MIGRATABLE,   // Process cannot be migrated
  MIGRATABLE,       // Process can be migrated
  BEING_EVICTED,    // Process being evicted from host
  BEING_IMPORTED,   // Process being imported to host
  MIGRATED_AWAY,    // Process migrated to another host
  HOME,             // Process on home host
};

//
// File System Information
//

struct FileInfo {
  uint32_t file_id;         // Unique file ID across network
  uint32_t server_id;       // File server ID
  uint32_t cache_blocks;    // Cached blocks on local client
  bool is_cached;           // File is cached locally
  bool is_dirty;            // Cache has modifications
  uint64_t file_size;       // File size in bytes
  char path[256];           // File path
};

//
// Virtual Memory Information
//

struct VMInfo {
  uint32_t num_pages;       // Number of pages
  uint32_t resident_pages;  // Pages in physical memory
  uint32_t cached_pages;    // Pages in file cache
  uint32_t remote_pages;    // Pages on remote hosts
  uint64_t page_faults;     // Page fault count
  uint64_t remote_faults;   // Remote page faults
};

//
// Host Information (for distributed system)
//

struct HostInfo {
  uint32_t host_id;         // Sprite host ID
  char hostname[256];       // Hostname
  uint32_t num_processors;  // Number of processors
  uint32_t num_processes;   // Number of processes
  uint32_t load_average;    // Load average (processes waiting)
  bool is_available;        // Host available for migration
  char kernel_version[64];  // Kernel version string
};

//
// Sprite Debugging Interface
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

  // For Sun-3 (m68k)
  static ErrorCode readM68kCPUState(ProcessId pid, Architecture::M68k::CPUState &state);
  static ErrorCode writeM68kCPUState(ProcessId pid, const Architecture::M68k::CPUState &state);

  // For DECstation (MIPS)
  static ErrorCode readMIPSCPUState(ProcessId pid, Architecture::MIPS::CPUState &state);
  static ErrorCode writeMIPSCPUState(ProcessId pid, const Architecture::MIPS::CPUState &state);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint32_t address);
  static ErrorCode removeBreakpoint(ProcessId pid, uint32_t address);

  //
  // Sprite-specific features
  //

  // Process information
  struct ProcessInfo {
    ProcessId pid;
    ProcessId ppid;         // Parent PID
    ProcessId family_id;    // Process family ID
    ProcessState state;     // Process state
    MigrationState migration_state; // Migration state
    uint32_t host_id;       // Current host ID
    uint32_t home_host_id;  // Home host ID
    uint32_t uid;           // User ID
    uint32_t gid;           // Group ID
    uint32_t num_migrations;// Number of times migrated
    char comm[16];          // Command name
  };

  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);

  // Process migration
  static ErrorCode migrateProcess(ProcessId pid, uint32_t target_host_id);
  static ErrorCode evictProcess(ProcessId pid);  // Force process to migrate
  static ErrorCode freezeProcess(ProcessId pid); // Freeze for migration
  static ErrorCode unfreezeProcess(ProcessId pid);

  // Distributed file system
  static ErrorCode getFileInfo(const char *path, FileInfo &info);
  static ErrorCode enumerateCachedFiles(std::vector<FileInfo> &files);
  static ErrorCode flushFileCache(const char *path);
  static ErrorCode invalidateFileCache(const char *path);

  // Virtual memory
  static ErrorCode getVMInfo(ProcessId pid, VMInfo &info);

  // Host information
  static ErrorCode getLocalHostInfo(HostInfo &info);
  static ErrorCode enumerateHosts(std::vector<HostInfo> &hosts);
  static ErrorCode getHostInfo(uint32_t host_id, HostInfo &info);

  // Load balancing
  struct LoadInfo {
    uint32_t host_id;
    uint32_t load_average;
    uint32_t num_processes;
    uint32_t num_idle_time;
    bool accept_migrations;
  };

  static ErrorCode getLoadInfo(uint32_t host_id, LoadInfo &info);
  static ErrorCode enumerateLoadInfo(std::vector<LoadInfo> &loads);

  // Prefix table (distributed naming)
  struct PrefixEntry {
    char prefix[256];       // Path prefix
    uint32_t server_id;     // File server ID
    char server_name[256];  // Server hostname
    bool is_local;          // Prefix refers to local server
  };

  static ErrorCode getPrefixTable(std::vector<PrefixEntry> &table);

  // Remote procedure calls
  struct RPCStats {
    uint64_t calls_sent;    // RPC calls sent
    uint64_t calls_received;// RPC calls received
    uint64_t retransmits;   // Retransmissions
    uint64_t timeouts;      // Timeouts
    uint64_t bytes_sent;    // Bytes sent
    uint64_t bytes_received;// Bytes received
  };

  static ErrorCode getRPCStats(RPCStats &stats);
};

//
// Sprite System Calls
//

namespace SystemCalls {
  // Standard Unix
  constexpr int SPRITE_exit        = 1;
  constexpr int SPRITE_fork        = 2;
  constexpr int SPRITE_read        = 3;
  constexpr int SPRITE_write       = 4;
  constexpr int SPRITE_open        = 5;
  constexpr int SPRITE_close       = 6;
  constexpr int SPRITE_wait        = 7;
  constexpr int SPRITE_creat       = 8;
  constexpr int SPRITE_link        = 9;
  constexpr int SPRITE_unlink      = 10;

  // Sprite-specific
  constexpr int SPRITE_migrate     = 100;  // Migrate process
  constexpr int SPRITE_getmachinfo = 101;  // Get machine information
  constexpr int SPRITE_setmigrate  = 102;  // Set migration state
  constexpr int SPRITE_getloadavg  = 103;  // Get load average
  constexpr int SPRITE_prefixload  = 104;  // Load prefix table
  constexpr int SPRITE_fs_prefix   = 105;  // File system prefix operations
  constexpr int SPRITE_vm_cmd      = 106;  // VM commands
}

//
// Sprite Signals
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
  constexpr int SIGURG     = 16;  // Urgent condition
  constexpr int SIGSTOP    = 17;  // Stop
  constexpr int SIGTSTP    = 18;  // Stop from tty
  constexpr int SIGCONT    = 19;  // Continue
  constexpr int SIGCHLD    = 20;  // Child status changed
  constexpr int SIGTTIN    = 21;  // Background read
  constexpr int SIGTTOU    = 22;  // Background write
  constexpr int SIGIO      = 23;  // I/O possible
  constexpr int SIGXCPU    = 24;  // CPU limit exceeded
  constexpr int SIGXFSZ    = 25;  // File size limit exceeded

  // Sprite-specific signals
  constexpr int SIGMIGRATE = 30;  // Process migration signal
  constexpr int SIGSUSPEND = 31;  // Process suspend for migration
  constexpr int SIGRESUME  = 32;  // Process resume after migration
}

//
// ptrace requests (Sprite-specific extensions)
//

#ifndef PTRACE_TRACEME
#define PTRACE_TRACEME      0   // Child declares it's being traced
#define PTRACE_PEEKTEXT     1   // Read word from text segment
#define PTRACE_PEEKDATA     2   // Read word from data segment
#define PTRACE_PEEKUSER     3   // Read word from user struct
#define PTRACE_POKETEXT     4   // Write word to text segment
#define PTRACE_POKEDATA     5   // Write word to data segment
#define PTRACE_POKEUSER     6   // Write word to user struct
#define PTRACE_CONT         7   // Continue execution
#define PTRACE_KILL         8   // Terminate process
#define PTRACE_SINGLESTEP   9   // Single step execution
#define PTRACE_ATTACH       10  // Attach to running process
#define PTRACE_DETACH       11  // Detach from process
#define PTRACE_GETMIGINFO   20  // Get migration info (Sprite-specific)
#define PTRACE_SETMIGINFO   21  // Set migration info (Sprite-specific)
#endif

} // namespace Sprite
} // namespace Host
} // namespace ds2
