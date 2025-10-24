//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Xinu - Educational Operating System ("Xinu Is Not Unix")
//
// Developed by Douglas Comer for teaching operating systems concepts
// Runs on various architectures: x86, ARM, MIPS, etc.
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace Xinu {

//
// Xinu Overview
//
// Xinu is an educational operating system developed by Douglas Comer:
// - "Xinu Is Not Unix" (recursive acronym)
// - Designed for teaching OS concepts
// - Simple, elegant design
// - Preemptive multitasking
// - Message passing IPC
// - Network stack (TCP/IP)
// - Runs on bare metal or embedded systems
// - Used in university OS courses worldwide
//

//
// Process States
//

enum class ProcessState {
  FREE,         // Process slot is free
  CURRENT,      // Process is currently running
  READY,        // Process is ready to run
  RECEIVE,      // Process waiting to receive message
  SLEEP,        // Process is sleeping
  SUSPENDED,    // Process is suspended
  WAITING,      // Process waiting on semaphore
  READING,      // Process waiting for read
  WRITING,      // Process waiting for write
};

//
// Process Control Block (PCB)
//

struct ProcessEntry {
  ProcessState state;       // Process state
  uint32_t priority;        // Process priority
  uint32_t *stack_base;     // Stack base address
  uint32_t *stack_ptr;      // Current stack pointer
  uint32_t stack_size;      // Stack size in bytes
  char name[32];            // Process name
  uint32_t sem;             // Semaphore if waiting
  uint32_t msg;             // Message if waiting
  uint32_t parent;          // Parent process ID
  uint32_t time;            // CPU time consumed
};

//
// Semaphore
//

struct Semaphore {
  int32_t count;            // Semaphore count
  uint32_t queue_head;      // Queue head
  uint32_t queue_tail;      // Queue tail
};

//
// Message Passing
//

constexpr uint32_t MSG_EMPTY = 0xFFFFFFFF;

//
// Xinu Debugging Interface
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
  static ErrorCode terminate(ProcessId pid);

  //
  // Memory operations
  //

  static ErrorCode readMemory(ProcessId pid, uint32_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(ProcessId pid, uint32_t address,
                               const void *buffer, size_t length);

  //
  // Process information
  //

  static ErrorCode getProcessEntry(ProcessId pid, ProcessEntry &entry);
  static ErrorCode enumerateProcesses(std::vector<ProcessEntry> &processes);

  //
  // Semaphore information
  //

  static ErrorCode getSemaphore(uint32_t sem_id, Semaphore &sem);
  static ErrorCode enumerateSemaphores(std::vector<Semaphore> &semaphores);

  //
  // Message passing
  //

  static ErrorCode getMessage(ProcessId pid, uint32_t &msg);
  static ErrorCode sendMessage(ProcessId pid, uint32_t msg);

  //
  // System state
  //

  struct SystemInfo {
    uint32_t num_processes;   // Number of processes
    uint32_t num_semaphores;  // Number of semaphores
    uint32_t current_pid;     // Currently running process
    uint32_t system_time;     // System time (ticks)
    uint32_t free_memory;     // Free memory in bytes
  };

  static ErrorCode getSystemInfo(SystemInfo &info);
};

//
// Xinu System Calls
//

namespace SystemCalls {
  constexpr int XINU_create    = 1;   // Create process
  constexpr int XINU_kill      = 2;   // Kill process
  constexpr int XINU_suspend   = 3;   // Suspend process
  constexpr int XINU_resume    = 4;   // Resume process
  constexpr int XINU_sleep     = 5;   // Sleep for time
  constexpr int XINU_receive   = 6;   // Receive message
  constexpr int XINU_send      = 7;   // Send message
  constexpr int XINU_wait      = 8;   // Wait on semaphore
  constexpr int XINU_signal    = 9;   // Signal semaphore
  constexpr int XINU_getpid    = 10;  // Get process ID
  constexpr int XINU_getprio   = 11;  // Get priority
  constexpr int XINU_chprio    = 12;  // Change priority
}

} // namespace Xinu
} // namespace Host
} // namespace ds2
