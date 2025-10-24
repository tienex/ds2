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
namespace BeOS {

// BeOS/Zeta/Haiku Debugging Support
// BeOS R5 (1999-2001), Zeta (2001-2007), Haiku (2008+)

// Team (process) ID
typedef int32_t team_id;

// Thread ID
typedef int32_t thread_id;

// Port ID (for messaging)
typedef int32_t port_id;

// Area (memory region) ID
typedef int32_t area_id;

// Team (process) information
struct team_info {
  team_id id;                    // Team ID
  thread_id thread_count;        // Number of threads
  area_id image_count;           // Number of images (executables/libraries)
  area_id area_count;            // Number of memory areas
  thread_id debugger_nub_thread; // Debugger nub thread ID
  port_id debugger_nub_port;     // Debugger nub port
  uint32_t flags;                // Team flags
  char name[256];                // Team name (executable path)
  uint64_t uid;                  // User ID
  uint64_t gid;                  // Group ID
};

// Thread information
struct thread_info {
  thread_id id;                  // Thread ID
  team_id team;                  // Owning team ID
  char name[32];                 // Thread name
  int32_t state;                 // Thread state
  int32_t priority;              // Thread priority
  uint64_t user_time;            // User CPU time
  uint64_t kernel_time;          // Kernel CPU time
  void *stack_base;              // Stack base address
  void *stack_end;               // Stack end address
};

// Thread states
enum thread_state {
  B_THREAD_RUNNING = 1,          // Currently running
  B_THREAD_READY = 2,            // Ready to run
  B_THREAD_RECEIVING = 3,        // Waiting for message
  B_THREAD_ASLEEP = 4,           // Sleeping
  B_THREAD_SUSPENDED = 5,        // Suspended
  B_THREAD_WAITING = 6,          // Waiting for event
};

// Image (executable/library) information
struct image_info {
  area_id id;                    // Image ID
  image_type type;               // Image type
  team_id team;                  // Team that loaded this image
  char name[256];                // Image path
  void *text;                    // Text segment address
  uint32_t text_size;            // Text segment size
  void *data;                    // Data segment address
  uint32_t data_size;            // Data segment size
  int32_t device;                // Device number
  uint64_t node;                 // Inode number
};

// Image types
enum image_type {
  B_APP_IMAGE = 1,               // Application
  B_LIBRARY_IMAGE = 2,           // Shared library
  B_ADD_ON_IMAGE = 3,            // Add-on (plugin)
  B_SYSTEM_IMAGE = 4,            // System library
};

// Area (memory region) information
struct area_info {
  area_id id;                    // Area ID
  char name[256];                // Area name
  uint32_t size;                 // Size in bytes
  uint32_t lock;                 // Lock type
  uint32_t protection;           // Protection flags
  team_id team;                  // Team ID
  void *address;                 // Address in memory
  uint32_t copy_count;           // Copy-on-write count
  uint32_t in_count;             // Number of times mapped in
  uint32_t out_count;            // Number of times mapped out
};

// Protection flags for areas
#define B_READ_AREA        0x01
#define B_WRITE_AREA       0x02
#define B_EXECUTE_AREA     0x04
#define B_STACK_AREA       0x08
#define B_CLONEABLE_AREA   0x10

// Debug events
enum debug_event_type {
  B_DEBUGGER_CALL = 1,           // debugger() call
  B_BREAKPOINT_HIT = 2,          // Breakpoint hit
  B_WATCHPOINT_HIT = 3,          // Watchpoint hit
  B_SINGLE_STEP = 4,             // Single step
  B_PRE_SYSCALL = 5,             // Before syscall
  B_POST_SYSCALL = 6,            // After syscall
  B_SIGNAL_RECEIVED = 7,         // Signal received
  B_THREAD_CREATED = 8,          // Thread created
  B_THREAD_DELETED = 9,          // Thread deleted
  B_TEAM_CREATED = 10,           // Team created
  B_TEAM_DELETED = 11,           // Team deleted
  B_TEAM_EXEC = 12,              // Team exec'd new image
  B_IMAGE_CREATED = 13,          // Image loaded
  B_IMAGE_DELETED = 14,          // Image unloaded
};

// Debug event structure
struct debug_event {
  debug_event_type type;         // Event type
  team_id team;                  // Team ID
  thread_id thread;              // Thread ID
  uint64_t timestamp;            // Event timestamp

  union {
    struct {
      void *address;             // Instruction address
      uint32_t signal;           // Signal number (if applicable)
    } debugger_call;

    struct {
      void *address;             // Breakpoint address
    } breakpoint;

    struct {
      void *address;             // Watchpoint address
      uint32_t type;             // Access type (read/write)
    } watchpoint;

    struct {
      int32_t syscall;           // Syscall number
      void *args;                // Syscall arguments
    } syscall;

    struct {
      int32_t signal;            // Signal number
    } signal;

    struct {
      thread_id new_thread;      // New thread ID
    } thread_created;

    struct {
      area_id image;             // Image ID
    } image_event;
  };
};

// Debugging operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Team (process) control
  static ErrorCode attachTeam(team_id team);
  static ErrorCode detachTeam(team_id team);
  static ErrorCode killTeam(team_id team);

  // Thread control
  static ErrorCode suspendThread(thread_id thread);
  static ErrorCode resumeThread(thread_id thread);
  static ErrorCode singleStepThread(thread_id thread);

  // Breakpoints
  static ErrorCode setBreakpoint(team_id team, Address address);
  static ErrorCode clearBreakpoint(team_id team, Address address);

  // Watchpoints
  static ErrorCode setWatchpoint(team_id team, Address address, uint32_t size,
                                 uint32_t type);
  static ErrorCode clearWatchpoint(team_id team, Address address);

  // Memory operations
  static ErrorCode readMemory(team_id team, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(team_id team, Address address,
                              void const *data, size_t size);

  // Register operations (architecture-specific)
  static ErrorCode readRegisters(thread_id thread, void *regs, size_t size);
  static ErrorCode writeRegisters(thread_id thread, void const *regs,
                                 size_t size);

  // Information queries
  static ErrorCode getTeamInfo(team_id team, team_info &info);
  static ErrorCode getThreadInfo(thread_id thread, thread_info &info);
  static ErrorCode getImageInfo(area_id image, image_info &info);
  static ErrorCode getAreaInfo(area_id area, area_info &info);

  // Event handling
  static ErrorCode waitForEvent(team_id team, debug_event &event,
                               uint64_t timeout_us);

  // Team/thread enumeration
  static ErrorCode getTeamThreads(team_id team,
                                 std::vector<thread_id> &threads);
  static ErrorCode getTeamImages(team_id team, std::vector<area_id> &images);
  static ErrorCode getTeamAreas(team_id team, std::vector<area_id> &areas);

  // Syscall tracing
  static ErrorCode enableSyscallTrace(team_id team, bool enable);

private:
  static bool _initialized;
};

} // namespace BeOS
} // namespace Host
} // namespace ds2
