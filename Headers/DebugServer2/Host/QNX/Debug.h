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
namespace QNX {

// QNX Neutrino RTOS Debugging Support
// QNX 4.x, QNX Neutrino 6.x, QNX 7.x

// Process ID
typedef int32_t pid_t;

// Thread ID
typedef int32_t tid_t;

// Channel ID (for message passing)
typedef int32_t chid_t;

// Connection ID
typedef int32_t coid_t;

// Debug attach flags
#define DEBUG_ATTACH_HOLD      0x0001  // Hold process on attach
#define DEBUG_ATTACH_RUN       0x0002  // Let process run
#define DEBUG_ATTACH_INHERIT   0x0004  // Inherit debug flags

// Debug run flags
#define DEBUG_RUN_STEP         0x0001  // Single step
#define DEBUG_RUN_STEP_OVER    0x0002  // Step over call
#define DEBUG_RUN_STEP_OUT     0x0004  // Step out of function
#define DEBUG_RUN_TRACE        0x0008  // Enable instruction trace
#define DEBUG_RUN_FAULT        0x0010  // Stop on fault
#define DEBUG_RUN_CLRSIG       0x0020  // Clear pending signal
#define DEBUG_RUN_CLRFLT       0x0040  // Clear pending fault

// Debug event types
enum debug_event_type {
  DEBUG_EVENT_BREAK = 1,         // Breakpoint hit
  DEBUG_EVENT_TRACE = 2,         // Single step
  DEBUG_EVENT_FAULT = 3,         // Fault occurred
  DEBUG_EVENT_SIGNAL = 4,        // Signal received
  DEBUG_EVENT_STOP = 5,          // Process stopped
  DEBUG_EVENT_EXIT = 6,          // Process exited
  DEBUG_EVENT_THREAD_CREATE = 7, // Thread created
  DEBUG_EVENT_THREAD_DESTROY = 8,// Thread destroyed
  DEBUG_EVENT_CHILD = 9,         // Child process
};

// Process information
struct procfs_info {
  pid_t pid;                     // Process ID
  pid_t parent;                  // Parent process ID
  pid_t pgrp;                    // Process group ID
  pid_t sid;                     // Session ID
  uint32_t flags;                // Process flags
  uint32_t umask;                // File mode creation mask
  uint32_t num_threads;          // Number of threads
  char name[256];                // Process name
};

// Thread information
struct procfs_status {
  tid_t tid;                     // Thread ID
  uint32_t state;                // Thread state
  uint32_t flags;                // Thread flags
  uint32_t why;                  // Why stopped
  uint32_t what;                 // What caused stop
  uint64_t ip;                   // Instruction pointer
  uint64_t sp;                   // Stack pointer
  uint32_t stksize;              // Stack size
  uint32_t tid_flags;            // Thread ID flags
  int32_t priority;              // Thread priority
  int32_t real_priority;         // Real-time priority
  int32_t policy;                // Scheduling policy
};

// Thread states
enum thread_state {
  STATE_DEAD = 0x00,             // Dead
  STATE_RUNNING = 0x01,          // Running
  STATE_READY = 0x02,            // Ready to run
  STATE_STOPPED = 0x03,          // Stopped (debugging)
  STATE_SEND = 0x04,             // Blocked on send
  STATE_RECEIVE = 0x05,          // Blocked on receive
  STATE_REPLY = 0x06,            // Blocked on reply
  STATE_STACK = 0x07,            // Stack fault
  STATE_WAITTHREAD = 0x08,       // Waiting for thread
  STATE_WAITPAGE = 0x09,         // Waiting for page
  STATE_SIGSUSPEND = 0x0A,       // Signal suspend
  STATE_SIGWAITINFO = 0x0B,      // Signal wait info
  STATE_NANOSLEEP = 0x0C,        // Nanosleep
  STATE_MUTEX = 0x0D,            // Blocked on mutex
  STATE_CONDVAR = 0x0E,          // Blocked on condvar
  STATE_JOIN = 0x0F,             // Blocked on join
  STATE_INTR = 0x10,             // Blocked on interrupt
  STATE_SEM = 0x11,              // Blocked on semaphore
};

// Why stopped
enum stop_why {
  STOP_REQUESTED = 1,            // Debug stop requested
  STOP_SIGNALLED = 2,            // Signal received
  STOP_FAULTED = 3,              // Fault occurred
  STOP_JOBCONTROL = 4,           // Job control
  STOP_TERMINATED = 5,           // Process terminated
};

// Memory map entry
struct procfs_mapinfo {
  uint64_t vaddr;                // Virtual address
  uint64_t size;                 // Size of mapping
  uint32_t flags;                // Mapping flags
  uint32_t dev;                  // Device
  uint64_t ino;                  // Inode
  uint64_t offset;               // File offset
};

// Memory mapping flags
#define MAP_PRIVATE    0x0001
#define MAP_SHARED     0x0002
#define MAP_FIXED      0x0004
#define MAP_ELF        0x0008
#define MAP_NOSYNCFILE 0x0010
#define MAP_LAZY       0x0020
#define MAP_STACK      0x0040
#define PROT_READ      0x0100
#define PROT_WRITE     0x0200
#define PROT_EXEC      0x0400

// Debug channel operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Process control
  static ErrorCode attachProcess(pid_t pid, uint32_t flags);
  static ErrorCode detachProcess(pid_t pid);
  static ErrorCode killProcess(pid_t pid);

  // Thread control
  static ErrorCode freezeThread(tid_t tid);
  static ErrorCode thawThread(tid_t tid);
  static ErrorCode runThread(tid_t tid, uint32_t flags);

  // Breakpoints and watchpoints
  static ErrorCode setBreakpoint(pid_t pid, Address address);
  static ErrorCode clearBreakpoint(pid_t pid, Address address);
  static ErrorCode setWatchpoint(pid_t pid, Address address, uint32_t size,
                                 uint32_t type);
  static ErrorCode clearWatchpoint(pid_t pid, Address address);

  // Memory operations
  static ErrorCode readMemory(pid_t pid, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(pid_t pid, Address address, void const *data,
                              size_t size);

  // Register operations (architecture-specific)
  static ErrorCode readRegisters(tid_t tid, void *regs, size_t size);
  static ErrorCode writeRegisters(tid_t tid, void const *regs, size_t size);

  // Information queries
  static ErrorCode getProcessInfo(pid_t pid, procfs_info &info);
  static ErrorCode getThreadStatus(tid_t tid, procfs_status &status);
  static ErrorCode getMemoryMaps(pid_t pid,
                                std::vector<procfs_mapinfo> &maps);

  // Thread enumeration
  static ErrorCode getThreads(pid_t pid, std::vector<tid_t> &threads);

  // Event handling
  static ErrorCode waitForDebugEvent(pid_t pid, debug_event_type &event,
                                    tid_t &tid, uint32_t timeout_ms);

  // Signal control
  static ErrorCode setSigMask(pid_t pid, uint64_t mask);
  static ErrorCode getSigMask(pid_t pid, uint64_t &mask);

  // QNX-specific: message passing debugging
  static ErrorCode getChannelInfo(pid_t pid, chid_t chid, void *info);
  static ErrorCode getConnectionInfo(pid_t pid, coid_t coid, void *info);

private:
  static bool _initialized;
};

} // namespace QNX
} // namespace Host
} // namespace ds2
