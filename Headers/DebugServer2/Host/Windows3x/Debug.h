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
namespace Windows3x {

// Windows 3.x Debugging Support (Windows 3.0, 3.1, 3.11)
// Supports both real mode (3.0) and protected mode (3.1+)
// Uses TOOLHELP.DLL for debugging API

// Task Database (TDB) structure - Windows task control block
struct TDB {
  uint16_t next_tdb;          // Selector of next TDB
  uint16_t task_pdb;          // Task's PSP selector
  uint16_t task_dta;          // Task's DTA selector
  uint16_t task_dta_offset;   // DTA offset
  uint8_t task_drive;         // Current drive
  char task_directory[65];    // Current directory
  uint16_t num_handles;       // Number of file handles
  uint16_t jft_selector;      // JFT selector
  uint16_t jft_size;          // JFT size
  uint16_t parent_tdb;        // Parent TDB selector
  uint16_t task_flags;        // Task flags
  uint16_t error_mode;        // Error mode
  uint16_t task_version;      // Expected Windows version
  uint16_t instance;          // Instance handle
  uint16_t module;            // Module handle
  uint16_t queue;             // Message queue
  uint16_t parent_queue;      // Parent message queue
  uint16_t sig_action;        // Signal action
  uint32_t sigint;            // Int 21h/23h handler
  uint32_t sigterm;           // Int 21h/24h handler
  uint16_t discard_level;     // Discard level
  uint8_t task_name[8];       // Task name
};

// Module Database (MDB) - Windows module information
struct MDB {
  uint16_t next_mdb;          // Next module selector
  uint16_t usage_count;       // Usage count
  char module_name[8];        // Module name
  uint16_t module_path_offset; // Offset to full path
  uint16_t ne_header_offset;  // NE header offset
};

// Global heap entry
struct GlobalEntry {
  uint32_t block_base;        // Block base address
  uint32_t block_size;        // Block size
  uint16_t handle;            // Global handle
  uint16_t owner;             // Owner task
  uint8_t lock_count;         // Lock count
  uint8_t page_lock_count;    // Page lock count
  uint8_t flags;              // Flags
  uint8_t heap_present;       // Heap present flag
};

// Local heap entry
struct LocalEntry {
  uint32_t address;           // Address
  uint16_t size;              // Size
  uint16_t handle;            // Handle
  uint8_t flags;              // Flags
  uint8_t lock_count;         // Lock count
  uint8_t type;               // Type
  uint16_t heap_segment;      // Heap segment
};

// Stack trace entry
struct StackFrame {
  uint16_t bp;                // Frame pointer
  uint16_t ip;                // Instruction pointer
  uint16_t cs;                // Code segment
  uint16_t ss;                // Stack segment
};

// TOOLHELP notification types
enum NotifyType {
  NFY_UNKNOWN = 0,            // Unknown
  NFY_LOADSEG = 1,            // Segment loaded
  NFY_FREESEG = 2,            // Segment freed
  NFY_STARTDLL = 3,           // DLL start
  NFY_STARTTASK = 4,          // Task start
  NFY_EXITTASK = 5,           // Task exit
  NFY_DELMODULE = 6,          // Module deleted
  NFY_RIP = 7,                // Fatal exit
  NFY_TASKIN = 8,             // Task switched in
  NFY_TASKOUT = 9,            // Task switched out
  NFY_INCHAR = 10,            // Character input
  NFY_OUTSTR = 11,            // String output
  NFY_LOGERROR = 12,          // Log error
  NFY_LOGPARAMERROR = 13,     // Log parameter error
};

// TOOLHELP functions
enum ToolHelpFunction {
  TH_GLOBALFIRST = 0x4E00,    // Get first global entry
  TH_GLOBALNEXT = 0x4E01,     // Get next global entry
  TH_GLOBALHANDLE = 0x4E02,   // Get global handle
  TH_GLOBALFREE = 0x4E03,     // Free global memory
  TH_LOCALFIRST = 0x4E04,     // Get first local entry
  TH_LOCALNEXT = 0x4E05,      // Get next local entry
  TH_MODULEFIRST = 0x4E06,    // Get first module
  TH_MODULENEXT = 0x4E07,     // Get next module
  TH_TASKFIRST = 0x4E08,      // Get first task
  TH_TASKNEXT = 0x4E09,       // Get next task
  TH_TASKFINDHANDLE = 0x4E0A, // Find task by handle
  TH_STACKTRACEFIRST = 0x4E0B, // Get first stack frame
  TH_STACKTRACENEXT = 0x4E0C, // Get next stack frame
  TH_NOTIFYREGISTER = 0x4E0D, // Register for notifications
  TH_NOTIFYUNREGISTER = 0x4E0E, // Unregister notifications
  TH_INTERRUPTREGISTER = 0x4E0F, // Register interrupt handler
  TH_INTERRUPTUNREGISTER = 0x4E10, // Unregister interrupt
  TH_TERMINATEAPP = 0x4E11,   // Terminate application
};

// Windows debugging modes
enum WindowsMode {
  WIN_REAL_MODE = 0,          // Real mode (Windows 3.0)
  WIN_STANDARD_MODE = 1,      // Standard mode (286)
  WIN_ENHANCED_MODE = 2,      // Enhanced mode (386)
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Mode detection
  static ErrorCode detectMode(WindowsMode &mode);
  static uint16_t getWindowsVersion();

  // Task (process) management
  static ErrorCode getTasks(std::vector<TDB> &tasks);
  static ErrorCode getTaskByHandle(uint16_t handle, TDB &task);
  static ErrorCode terminateTask(uint16_t task_handle);
  static ErrorCode switchToTask(uint16_t task_handle);

  // Module management
  static ErrorCode getModules(std::vector<MDB> &modules);
  static ErrorCode getModuleHandle(const char *name, uint16_t &handle);

  // Memory inspection
  static ErrorCode enumerateGlobalHeap(std::vector<GlobalEntry> &entries);
  static ErrorCode enumerateLocalHeap(uint16_t segment,
                                     std::vector<LocalEntry> &entries);
  static ErrorCode getGlobalEntry(uint16_t handle, GlobalEntry &entry);

  // Stack walking
  static ErrorCode getStackTrace(uint16_t task, std::vector<StackFrame> &frames);

  // Breakpoints
  static ErrorCode setBreakpoint(uint16_t selector, uint16_t offset);
  static ErrorCode clearBreakpoint(uint16_t selector, uint16_t offset);

  // Memory operations
  static ErrorCode readMemory(uint16_t selector, uint16_t offset,
                             void *data, size_t size);
  static ErrorCode writeMemory(uint16_t selector, uint16_t offset,
                              const void *data, size_t size);

  // Register access
  static ErrorCode getRegisters(uint16_t task, void *registers, size_t size);
  static ErrorCode setRegisters(uint16_t task, const void *registers, size_t size);

  // Notification callbacks
  typedef void (*NotifyCallback)(NotifyType type, uint16_t param1, uint16_t param2);
  static ErrorCode registerNotify(NotifyCallback callback);
  static ErrorCode unregisterNotify(NotifyCallback callback);

  // Interrupt handling
  static ErrorCode registerInterruptHandler(uint8_t interrupt, void *handler);
  static ErrorCode unregisterInterruptHandler(uint8_t interrupt);

  // Windows API tracing
  static ErrorCode enableAPITrace(bool enable);
  static ErrorCode setAPIBreakpoint(const char *module, const char *function);

  // Message queue inspection
  static ErrorCode getMessageQueue(uint16_t task, std::vector<void*> &messages);

  // GDI/USER object tracking
  static ErrorCode getGDIObjects(uint16_t task, std::vector<uint16_t> &handles);
  static ErrorCode getUserObjects(uint16_t task, std::vector<uint16_t> &handles);

private:
  static bool _initialized;
  static WindowsMode _current_mode;
};

} // namespace Windows3x
} // namespace Host
} // namespace ds2
