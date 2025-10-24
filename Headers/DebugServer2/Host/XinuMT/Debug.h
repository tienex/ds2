//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// XinuMT - Multithreaded variant of Xinu
//
// Educational OS with native threading support
//

#pragma once

#include "DebugServer2/Base.h"
#include "DebugServer2/Host/Xinu/Debug.h"

namespace ds2 {
namespace Host {
namespace XinuMT {

//
// XinuMT Overview
//
// XinuMT extends Xinu with multithreading support:
// - Multiple threads per process
// - Thread-local storage
// - Thread synchronization primitives
// - SMP (Symmetric Multi-Processing) support
// - POSIX threads API compatibility
// - Maintains Xinu's simplicity for education
//

//
// Thread States
//

enum class ThreadState {
  FREE,         // Thread slot is free
  CURRENT,      // Thread is currently running
  READY,        // Thread is ready to run
  BLOCKED,      // Thread is blocked
  SLEEPING,     // Thread is sleeping
  WAITING,      // Thread waiting on condition
  SUSPENDED,    // Thread is suspended
  TERMINATED,   // Thread has terminated
};

//
// Thread Control Block (TCB)
//

struct ThreadEntry {
  uint32_t thread_id;       // Thread ID
  ProcessId process_id;     // Process ID (owner)
  ThreadState state;        // Thread state
  uint32_t priority;        // Thread priority
  uint32_t *stack_base;     // Stack base address
  uint32_t *stack_ptr;      // Current stack pointer
  uint32_t stack_size;      // Stack size in bytes
  char name[32];            // Thread name
  void *tls;                // Thread-local storage
  uint32_t cpu_affinity;    // CPU affinity mask
  uint32_t cpu_time;        // CPU time consumed
};

//
// Synchronization Primitives
//

// Mutex
struct Mutex {
  uint32_t owner_thread;    // Thread that owns the mutex
  bool is_locked;           // Lock state
  uint32_t queue_head;      // Waiting threads queue
  uint32_t recursive_count; // Recursive lock count
};

// Condition variable
struct ConditionVariable {
  uint32_t queue_head;      // Waiting threads queue
  uint32_t queue_tail;      // Queue tail
  uint32_t associated_mutex;// Associated mutex
};

// Read-Write lock
struct RWLock {
  uint32_t readers;         // Number of readers
  uint32_t writer_thread;   // Writer thread (0 if none)
  uint32_t reader_queue;    // Reader queue
  uint32_t writer_queue;    // Writer queue
};

// Barrier
struct Barrier {
  uint32_t count;           // Required thread count
  uint32_t arrived;         // Threads arrived
  uint32_t queue_head;      // Waiting threads
};

//
// SMP Support
//

struct CPUInfo {
  uint32_t cpu_id;          // CPU ID
  bool is_online;           // CPU online status
  uint32_t current_thread;  // Current thread on this CPU
  uint32_t idle_time;       // Idle time
  uint32_t run_queue_length;// Run queue length
};

//
// XinuMT Debugging Interface
//

class Debug {
public:
  //
  // Thread control
  //

  static ErrorCode attachThread(uint32_t thread_id);
  static ErrorCode detachThread(uint32_t thread_id);
  static ErrorCode suspendThread(uint32_t thread_id);
  static ErrorCode resumeThread(uint32_t thread_id);
  static ErrorCode terminateThread(uint32_t thread_id);

  //
  // Process control (inherited from Xinu)
  //

  static ErrorCode attach(ProcessId pid);
  static ErrorCode detach(ProcessId pid);
  static ErrorCode suspend(ProcessId pid);
  static ErrorCode resume(ProcessId pid);
  static ErrorCode terminate(ProcessId pid);

  //
  // Memory operations
  //

  static ErrorCode readMemory(uint32_t thread_id, uint32_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(uint32_t thread_id, uint32_t address,
                               const void *buffer, size_t length);

  //
  // Thread information
  //

  static ErrorCode getThreadEntry(uint32_t thread_id, ThreadEntry &entry);
  static ErrorCode enumerateThreads(ProcessId pid, std::vector<ThreadEntry> &threads);
  static ErrorCode enumerateAllThreads(std::vector<ThreadEntry> &threads);

  //
  // Process information (inherited)
  //

  static ErrorCode getProcessEntry(ProcessId pid, Xinu::ProcessEntry &entry);
  static ErrorCode enumerateProcesses(std::vector<Xinu::ProcessEntry> &processes);

  //
  // Synchronization primitives
  //

  static ErrorCode getMutex(uint32_t mutex_id, Mutex &mutex);
  static ErrorCode getConditionVariable(uint32_t cv_id, ConditionVariable &cv);
  static ErrorCode getRWLock(uint32_t rwlock_id, RWLock &rwlock);
  static ErrorCode getBarrier(uint32_t barrier_id, Barrier &barrier);

  //
  // SMP information
  //

  static ErrorCode getCPUInfo(uint32_t cpu_id, CPUInfo &info);
  static ErrorCode enumerateCPUs(std::vector<CPUInfo> &cpus);

  //
  // System state
  //

  struct SystemInfo {
    uint32_t num_processes;   // Number of processes
    uint32_t num_threads;     // Number of threads
    uint32_t num_cpus;        // Number of CPUs
    uint32_t num_mutexes;     // Number of mutexes
    uint32_t num_cvs;         // Number of condition variables
    uint32_t system_time;     // System time (ticks)
    uint32_t free_memory;     // Free memory in bytes
  };

  static ErrorCode getSystemInfo(SystemInfo &info);

  //
  // Thread-local storage
  //

  static ErrorCode getTLS(uint32_t thread_id, void **tls);
  static ErrorCode setTLS(uint32_t thread_id, void *tls);
};

//
// XinuMT System Calls (extends Xinu)
//

namespace SystemCalls {
  // Thread management
  constexpr int XINUMT_thread_create  = 100;  // Create thread
  constexpr int XINUMT_thread_exit    = 101;  // Exit thread
  constexpr int XINUMT_thread_join    = 102;  // Join thread
  constexpr int XINUMT_thread_detach  = 103;  // Detach thread
  constexpr int XINUMT_thread_yield   = 104;  // Yield CPU
  constexpr int XINUMT_thread_self    = 105;  // Get thread ID

  // Mutex operations
  constexpr int XINUMT_mutex_init     = 110;  // Initialize mutex
  constexpr int XINUMT_mutex_destroy  = 111;  // Destroy mutex
  constexpr int XINUMT_mutex_lock     = 112;  // Lock mutex
  constexpr int XINUMT_mutex_trylock  = 113;  // Try lock mutex
  constexpr int XINUMT_mutex_unlock   = 114;  // Unlock mutex

  // Condition variables
  constexpr int XINUMT_cond_init      = 120;  // Initialize CV
  constexpr int XINUMT_cond_destroy   = 121;  // Destroy CV
  constexpr int XINUMT_cond_wait      = 122;  // Wait on CV
  constexpr int XINUMT_cond_signal    = 123;  // Signal CV
  constexpr int XINUMT_cond_broadcast = 124;  // Broadcast CV

  // RW locks
  constexpr int XINUMT_rwlock_init    = 130;  // Initialize RW lock
  constexpr int XINUMT_rwlock_destroy = 131;  // Destroy RW lock
  constexpr int XINUMT_rwlock_rdlock  = 132;  // Read lock
  constexpr int XINUMT_rwlock_wrlock  = 133;  // Write lock
  constexpr int XINUMT_rwlock_unlock  = 134;  // Unlock

  // Barriers
  constexpr int XINUMT_barrier_init   = 140;  // Initialize barrier
  constexpr int XINUMT_barrier_destroy= 141;  // Destroy barrier
  constexpr int XINUMT_barrier_wait   = 142;  // Wait on barrier

  // TLS
  constexpr int XINUMT_tls_create     = 150;  // Create TLS key
  constexpr int XINUMT_tls_get        = 151;  // Get TLS value
  constexpr int XINUMT_tls_set        = 152;  // Set TLS value
}

} // namespace XinuMT
} // namespace Host
} // namespace ds2
