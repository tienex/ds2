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
namespace NetWare {

// NetWare Server Debugging Support (3.x, 4.x, 5.x, 6.x)
// Uses NetWare Debug API and NLM loader

// Thread ID (NetWare uses thread-based model, not processes)
typedef uint32_t thread_t;

// NLM handle
typedef uint32_t nlm_handle_t;

// Screen handle
typedef uint32_t screen_t;

// Thread states
enum ThreadState {
  THREAD_STATE_RUNNING = 0,
  THREAD_STATE_READY = 1,
  THREAD_STATE_WAITING = 2,
  THREAD_STATE_SLEEPING = 3,
  THREAD_STATE_SUSPENDED = 4,
  THREAD_STATE_DEAD = 5,
};

// Thread priorities
enum ThreadPriority {
  PRIORITY_LOWEST = 0,
  PRIORITY_LOW = 32,
  PRIORITY_NORMAL = 64,
  PRIORITY_HIGH = 96,
  PRIORITY_HIGHEST = 127,
};

// Thread information
struct ThreadInfo {
  thread_t thread_id;
  char name[128];
  ThreadState state;
  uint8_t priority;
  uint32_t stack_size;
  uint32_t stack_ptr;
  nlm_handle_t nlm_handle;
  uint32_t entry_point;
};

// NLM information
struct NLMInfo {
  nlm_handle_t handle;
  char name[128];
  char file_path[256];
  uint32_t code_size;
  uint32_t data_size;
  uint32_t bss_size;
  uint32_t code_start;
  uint32_t data_start;
  uint32_t entry_point;
  uint32_t flags;
  uint32_t ref_count;
};

// Resource tag information
struct ResourceTag {
  char signature[16];
  uint32_t count;
  uint32_t data_ptr;
};

// Debug event types
enum DebugEventType {
  EVENT_BREAKPOINT = 1,
  EVENT_WATCHPOINT = 2,
  EVENT_SINGLE_STEP = 3,
  EVENT_NLM_LOAD = 4,
  EVENT_NLM_UNLOAD = 5,
  EVENT_THREAD_CREATE = 6,
  EVENT_THREAD_EXIT = 7,
  EVENT_EXCEPTION = 8,
};

// Debug event structure
struct DebugEvent {
  DebugEventType type;
  thread_t thread_id;
  uint32_t address;
  uint32_t data;
};

// NetWare exception types
enum ExceptionType {
  EXC_DIVIDE_BY_ZERO = 0,
  EXC_DEBUG = 1,
  EXC_NMI = 2,
  EXC_BREAKPOINT = 3,
  EXC_OVERFLOW = 4,
  EXC_BOUND_CHECK = 5,
  EXC_INVALID_OPCODE = 6,
  EXC_COPROCESSOR_NOT_AVAILABLE = 7,
  EXC_DOUBLE_FAULT = 8,
  EXC_COPROCESSOR_SEGMENT_OVERRUN = 9,
  EXC_INVALID_TSS = 10,
  EXC_SEGMENT_NOT_PRESENT = 11,
  EXC_STACK_FAULT = 12,
  EXC_GENERAL_PROTECTION = 13,
  EXC_PAGE_FAULT = 14,
  EXC_COPROCESSOR_ERROR = 16,
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Server information
  static ErrorCode getServerVersion(uint32_t &major, uint32_t &minor, uint32_t &revision);
  static ErrorCode getServerName(std::string &name);

  // Thread management
  static ErrorCode getThreads(std::vector<thread_t> &threads);
  static ErrorCode getThreadInfo(thread_t thread, ThreadInfo &info);
  static ErrorCode suspendThread(thread_t thread);
  static ErrorCode resumeThread(thread_t thread);
  static ErrorCode terminateThread(thread_t thread);
  static ErrorCode setThreadPriority(thread_t thread, uint8_t priority);

  // NLM management
  static ErrorCode getNLMs(std::vector<nlm_handle_t> &nlms);
  static ErrorCode getNLMInfo(nlm_handle_t handle, NLMInfo &info);
  static ErrorCode loadNLM(const char *path, nlm_handle_t &handle);
  static ErrorCode unloadNLM(nlm_handle_t handle);

  // Execution control
  static ErrorCode continueThread(thread_t thread);
  static ErrorCode singleStep(thread_t thread);

  // Breakpoints
  static ErrorCode setBreakpoint(uint32_t address);
  static ErrorCode clearBreakpoint(uint32_t address);

  // Watchpoints (debug registers on 386+)
  static ErrorCode setWatchpoint(uint32_t address, uint32_t size, uint32_t type);
  static ErrorCode clearWatchpoint(uint32_t address);

  // Memory operations
  static ErrorCode readMemory(uint32_t address, void *data, size_t size);
  static ErrorCode writeMemory(uint32_t address, const void *data, size_t size);

  // Register access
  static ErrorCode getRegisters(thread_t thread, void *registers, size_t size);
  static ErrorCode setRegisters(thread_t thread, const void *registers, size_t size);

  // Stack operations
  static ErrorCode getStackTrace(thread_t thread, std::vector<uint32_t> &frames);
  static ErrorCode getStackInfo(thread_t thread, uint32_t &stack_base,
                               uint32_t &stack_size, uint32_t &stack_ptr);

  // Symbol lookup (using NLM public symbols)
  static ErrorCode getSymbolAddress(nlm_handle_t nlm, const char *symbol,
                                   uint32_t &address);
  static ErrorCode getSymbolName(uint32_t address, std::string &name,
                                uint32_t &offset);

  // Resource management
  static ErrorCode getResourceTags(nlm_handle_t nlm,
                                  std::vector<ResourceTag> &tags);

  // Console/screen management
  static ErrorCode getConsoleScreen(screen_t &screen);
  static ErrorCode createScreen(const char *name, screen_t &screen);
  static ErrorCode destroyScreen(screen_t screen);

  // Event handling
  static ErrorCode waitForEvent(DebugEvent &event, uint32_t timeout_ms);

  // Exception handling
  static ErrorCode setExceptionHandler(ExceptionType type, void *handler);
  static ErrorCode clearExceptionHandler(ExceptionType type);

  // Server statistics
  static ErrorCode getCacheStatistics(uint32_t &buffer_count, uint32_t &dirty_buffers);
  static ErrorCode getConnectionCount(uint32_t &count);
  static ErrorCode getMemoryStatistics(uint32_t &total, uint32_t &available);

private:
  static bool _initialized;
};

} // namespace NetWare
} // namespace Host
} // namespace ds2
