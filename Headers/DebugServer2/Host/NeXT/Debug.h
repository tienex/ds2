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
namespace NeXT {

// NeXT/Apple Mach-based OS Debugging Support
// NeXTSTEP, OpenStep, Rhapsody, Mac OS X Server, NeXT Mach
// All use Mach kernel debugging interfaces

// Task (process) and thread ports
typedef uint32_t mach_port_t;
typedef mach_port_t task_port_t;
typedef mach_port_t thread_port_t;

// Task and thread IDs
typedef int32_t pid_t;
typedef int32_t tid_t;

// Mach exception types
enum mach_exception_type {
  EXC_BAD_ACCESS = 1,            // Could not access memory
  EXC_BAD_INSTRUCTION = 2,       // Instruction failed
  EXC_ARITHMETIC = 3,            // Arithmetic exception
  EXC_EMULATION = 4,             // Emulation instruction
  EXC_SOFTWARE = 5,              // Software exception
  EXC_BREAKPOINT = 6,            // Trace, breakpoint, etc.
  EXC_SYSCALL = 7,               // System call redirection
  EXC_MACH_SYSCALL = 8,          // Mach system call
  EXC_RPC_ALERT = 9,             // RPC alert
};

// Thread states
enum thread_state {
  TH_STATE_RUNNING = 1,          // Running
  TH_STATE_STOPPED = 2,          // Stopped
  TH_STATE_WAITING = 3,          // Waiting
  TH_STATE_UNINTERRUPTIBLE = 4,  // Uninterruptible
  TH_STATE_HALTED = 5,           // Halted at clean point
};

// Thread basic info
struct thread_basic_info {
  uint32_t user_time;            // User run time (seconds)
  uint32_t system_time;          // System run time (seconds)
  int32_t cpu_usage;             // CPU usage percentage
  int32_t policy;                // Scheduling policy
  int32_t run_state;             // Thread state
  int32_t flags;                 // Thread flags
  int32_t suspend_count;         // Suspend count
  int32_t sleep_time;            // Sleep time
};

// Task basic info
struct task_basic_info {
  int32_t suspend_count;         // Suspend count
  uint32_t virtual_size;         // Virtual memory size
  uint32_t resident_size;        // Resident memory size
  uint64_t user_time;            // User time
  uint64_t system_time;          // System time
  int32_t policy;                // Default scheduling policy
};

// VM region info
struct vm_region_info {
  uint64_t address;              // Starting address
  uint64_t size;                 // Size of region
  uint32_t protection;           // Memory protection
  uint32_t max_protection;       // Maximum protection
  uint32_t inheritance;          // Inheritance
  int32_t is_shared;             // Shared flag
  int32_t reserved;              // Reserved
  uint64_t offset;               // Offset into object
};

// Memory protection flags
#define VM_PROT_NONE       0x00
#define VM_PROT_READ       0x01
#define VM_PROT_WRITE      0x02
#define VM_PROT_EXECUTE    0x04

// Debug exceptions
struct debug_exception {
  thread_port_t thread;          // Thread that caused exception
  mach_exception_type type;      // Exception type
  uint64_t codes[2];             // Exception codes
  uint32_t code_count;           // Number of codes
};

// Debugging operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Task (process) control
  static ErrorCode taskForPid(pid_t pid, task_port_t &task);
  static ErrorCode suspendTask(task_port_t task);
  static ErrorCode resumeTask(task_port_t task);
  static ErrorCode terminateTask(task_port_t task);

  // Thread control
  static ErrorCode getThreads(task_port_t task,
                             std::vector<thread_port_t> &threads);
  static ErrorCode suspendThread(thread_port_t thread);
  static ErrorCode resumeThread(thread_port_t thread);
  static ErrorCode abortThread(thread_port_t thread);

  // Breakpoints and watchpoints
  static ErrorCode setBreakpoint(task_port_t task, Address address);
  static ErrorCode clearBreakpoint(task_port_t task, Address address);
  static ErrorCode setWatchpoint(task_port_t task, Address address,
                                 uint32_t size, uint32_t type);
  static ErrorCode clearWatchpoint(task_port_t task, Address address);

  // Memory operations
  static ErrorCode readMemory(task_port_t task, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(task_port_t task, Address address,
                              void const *data, size_t size);
  static ErrorCode protectMemory(task_port_t task, Address address,
                                size_t size, uint32_t protection);

  // Register operations (architecture-specific)
  static ErrorCode getThreadState(thread_port_t thread, void *state,
                                 size_t &size);
  static ErrorCode setThreadState(thread_port_t thread, void const *state,
                                 size_t size);

  // Information queries
  static ErrorCode getTaskInfo(task_port_t task, task_basic_info &info);
  static ErrorCode getThreadInfo(thread_port_t thread,
                                thread_basic_info &info);
  static ErrorCode getVMRegions(task_port_t task,
                               std::vector<vm_region_info> &regions);

  // Exception handling
  static ErrorCode catchExceptions(task_port_t task);
  static ErrorCode waitForException(task_port_t task,
                                   debug_exception &exception,
                                   uint32_t timeout_ms);

  // Port management
  static ErrorCode deallocatePort(mach_port_t port);

  // Architecture-specific state flavors
  static constexpr int THREAD_STATE_FLAVOR_GENERAL = 1;
  static constexpr int THREAD_STATE_FLAVOR_FLOAT = 2;
  static constexpr int THREAD_STATE_FLAVOR_DEBUG = 3;

private:
  static bool _initialized;
};

} // namespace NeXT
} // namespace Host
} // namespace ds2
