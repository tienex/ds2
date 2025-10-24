//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Fuchsia - Google's capability-based operating system
//
// Based on Zircon microkernel, supports x86-64, ARM64, RISC-V
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace Fuchsia {

//
// Fuchsia Overview
//
// Fuchsia is Google's open-source, capability-based operating system:
// - Zircon microkernel (not Linux-based)
// - Capability-based security model
// - Component-based architecture
// - FIDL (Fuchsia Interface Definition Language) for IPC
// - Supports x86-64, ARM64, RISC-V
// - No global filesystem namespace
// - Everything is a component
//

//
// Zircon Kernel Objects
//

using zx_handle_t = uint32_t;
using zx_koid_t = uint64_t;
using zx_status_t = int32_t;

enum class ObjectType {
  NONE,
  PROCESS,
  THREAD,
  VMO,          // Virtual Memory Object
  CHANNEL,
  EVENT,
  PORT,
  INTERRUPT,
  PCI_DEVICE,
  LOG,
  SOCKET,
  RESOURCE,
  EVENTPAIR,
  JOB,
  VMAR,         // Virtual Memory Address Region
  FIFO,
  GUEST,
  VCPU,
  TIMER,
  IOMMU,
  BTI,          // Bus Transaction Initiator
  PROFILE,
  PMT,          // Pinned Memory Token
  SUSPEND_TOKEN,
  PAGER,
  EXCEPTION,
  CLOCK,
  STREAM,
  MSI,
};

//
// Thread States
//

enum class ThreadState {
  NEW,
  RUNNING,
  SUSPENDED,
  BLOCKED,
  DYING,
  DEAD,
  BLOCKED_EXCEPTION,
  BLOCKED_SLEEPING,
  BLOCKED_FUTEX,
  BLOCKED_PORT,
  BLOCKED_CHANNEL,
  BLOCKED_WAIT_ONE,
  BLOCKED_WAIT_MANY,
  BLOCKED_INTERRUPT,
  BLOCKED_PAGER,
};

//
// Exception Types
//

enum class ExceptionType {
  FATAL_PAGE_FAULT,
  UNDEFINED_INSTRUCTION,
  GENERAL,
  SW_BREAKPOINT,
  HW_BREAKPOINT,
  UNALIGNED_ACCESS,
  THREAD_STARTING,
  THREAD_EXITING,
  POLICY_ERROR,
  PROCESS_STARTING,
};

//
// Zircon Handles
//

struct Handle {
  zx_handle_t handle;
  zx_koid_t koid;
  ObjectType type;
  uint32_t rights;
  uint32_t related_koid;
  uint32_t peer_owner_koid;
};

//
// Process Information
//

struct ProcessInfo {
  zx_koid_t koid;
  char name[32];
  uint32_t return_code;
  uint64_t started;
  uint32_t flags;
};

//
// Thread Information
//

struct ThreadInfo {
  zx_koid_t koid;
  char name[32];
  ThreadState state;
  uint32_t wait_exception_channel_type;
  uint32_t flags;
};

//
// VMO (Virtual Memory Object) Information
//

struct VMOInfo {
  zx_koid_t koid;
  char name[32];
  uint64_t size_bytes;
  zx_koid_t parent_koid;
  uint64_t num_children;
  uint64_t num_mappings;
  uint64_t share_count;
  uint32_t flags;
  uint64_t committed_bytes;
  uint32_t cache_policy;
};

//
// Fuchsia Debugging Interface
//

class Debug {
public:
  //
  // Process control
  //

  static ErrorCode attach(zx_koid_t process_koid, zx_handle_t *debug_handle);
  static ErrorCode detach(zx_handle_t debug_handle);
  static ErrorCode suspend(zx_handle_t thread_handle, zx_handle_t *suspend_token);
  static ErrorCode resume(zx_handle_t suspend_token);
  static ErrorCode kill(zx_handle_t process_handle);

  //
  // Thread control
  //

  static ErrorCode suspendThread(zx_handle_t thread_handle, zx_handle_t *suspend_token);
  static ErrorCode resumeThread(zx_handle_t suspend_token);
  static ErrorCode readThreadState(zx_handle_t thread_handle, uint32_t kind, void *buffer, size_t length);
  static ErrorCode writeThreadState(zx_handle_t thread_handle, uint32_t kind, const void *buffer, size_t length);

  //
  // Memory operations
  //

  static ErrorCode readMemory(zx_handle_t process_handle, uint64_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(zx_handle_t process_handle, uint64_t address,
                               const void *buffer, size_t length);

  //
  // Breakpoints and exceptions
  //

  static ErrorCode setBreakpoint(zx_handle_t thread_handle, uint64_t address);
  static ErrorCode removeBreakpoint(zx_handle_t thread_handle, uint64_t address);

  static ErrorCode setHardwareBreakpoint(zx_handle_t thread_handle, uint64_t address);
  static ErrorCode removeHardwareBreakpoint(zx_handle_t thread_handle, uint64_t address);

  static ErrorCode setWatchpoint(zx_handle_t thread_handle, uint64_t address, size_t size);
  static ErrorCode removeWatchpoint(zx_handle_t thread_handle, uint64_t address);

  //
  // Exception handling
  //

  static ErrorCode waitForException(zx_handle_t exception_channel, zx_handle_t *exception);
  static ErrorCode getExceptionInfo(zx_handle_t exception, ExceptionType &type, zx_koid_t &thread_koid);
  static ErrorCode resumeFromException(zx_handle_t thread_handle, uint32_t options);

  //
  // Object enumeration
  //

  static ErrorCode enumerateProcesses(std::vector<ProcessInfo> &processes);
  static ErrorCode enumerateThreads(zx_handle_t process_handle, std::vector<ThreadInfo> &threads);
  static ErrorCode enumerateHandles(zx_handle_t process_handle, std::vector<Handle> &handles);
  static ErrorCode enumerateVMOs(zx_handle_t process_handle, std::vector<VMOInfo> &vmos);

  //
  // Component debugging
  //

  struct ComponentInfo {
    char name[256];
    char url[512];
    zx_koid_t process_koid;
    bool is_running;
  };

  static ErrorCode enumerateComponents(std::vector<ComponentInfo> &components);

  //
  // FIDL debugging
  //

  struct FIDLChannel {
    zx_handle_t handle;
    char protocol[256];
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
  };

  static ErrorCode enumerateFIDLChannels(zx_handle_t process_handle, std::vector<FIDLChannel> &channels);

  //
  // Job control
  //

  struct JobInfo {
    zx_koid_t koid;
    char name[32];
    uint32_t return_code;
    uint32_t flags;
  };

  static ErrorCode enumerateJobs(std::vector<JobInfo> &jobs);
  static ErrorCode getJobInfo(zx_handle_t job_handle, JobInfo &info);
};

//
// Zircon System Calls (subset)
//

namespace SystemCalls {
  constexpr int ZX_process_create       = 0;
  constexpr int ZX_process_start        = 1;
  constexpr int ZX_process_read_memory  = 2;
  constexpr int ZX_process_write_memory = 3;
  constexpr int ZX_thread_create        = 10;
  constexpr int ZX_thread_start         = 11;
  constexpr int ZX_thread_read_state    = 12;
  constexpr int ZX_thread_write_state   = 13;
  constexpr int ZX_channel_create       = 20;
  constexpr int ZX_channel_read         = 21;
  constexpr int ZX_channel_write        = 22;
  constexpr int ZX_vmo_create           = 30;
  constexpr int ZX_vmo_read             = 31;
  constexpr int ZX_vmo_write            = 32;
  constexpr int ZX_port_create          = 40;
  constexpr int ZX_port_queue           = 41;
  constexpr int ZX_port_wait            = 42;
  constexpr int ZX_exception_get_thread = 50;
  constexpr int ZX_exception_get_process= 51;
}

//
// Zircon Status Codes
//

namespace Status {
  constexpr zx_status_t OK                     = 0;
  constexpr zx_status_t ERR_INTERNAL           = -1;
  constexpr zx_status_t ERR_NOT_SUPPORTED      = -2;
  constexpr zx_status_t ERR_NO_RESOURCES       = -3;
  constexpr zx_status_t ERR_NO_MEMORY          = -4;
  constexpr zx_status_t ERR_INVALID_ARGS       = -10;
  constexpr zx_status_t ERR_BAD_HANDLE         = -11;
  constexpr zx_status_t ERR_WRONG_TYPE         = -12;
  constexpr zx_status_t ERR_BAD_SYSCALL        = -13;
  constexpr zx_status_t ERR_OUT_OF_RANGE       = -14;
  constexpr zx_status_t ERR_BUFFER_TOO_SMALL   = -15;
  constexpr zx_status_t ERR_NOT_FOUND          = -25;
  constexpr zx_status_t ERR_ALREADY_EXISTS     = -26;
  constexpr zx_status_t ERR_ALREADY_BOUND      = -27;
  constexpr zx_status_t ERR_UNAVAILABLE        = -28;
  constexpr zx_status_t ERR_ACCESS_DENIED      = -30;
  constexpr zx_status_t ERR_TIMED_OUT          = -40;
  constexpr zx_status_t ERR_SHOULD_WAIT        = -41;
  constexpr zx_status_t ERR_CANCELED           = -42;
}

} // namespace Fuchsia
} // namespace Host
} // namespace ds2
