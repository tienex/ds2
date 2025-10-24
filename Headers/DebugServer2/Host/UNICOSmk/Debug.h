//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UNICOS/mk - Microkernel-based UNICOS for Massively Parallel Systems
//
// UNICOS/mk ran on:
// - Cray T3D (DEC Alpha 21064 processors, up to 2048 PEs)
// - Cray T3E (DEC Alpha 21164 processors, up to 2176 PEs)
// - Cray SV1 (vector processors with UNICOS/mk option)
// - Cray X1 (vector + MSP architecture)
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {
namespace UNICOSmk {

//
// UNICOS/mk Overview
//
// UNICOS/mk is a microkernel-based operating system designed for
// massively parallel processing (MPP) systems:
// - Mach-based microkernel
// - Distributed shared memory across thousands of processing elements
// - NUMA (Non-Uniform Memory Access) architecture
// - Message passing interface (MPI)
// - Global virtual address space
// - Per-PE local memory and global memory
//

//
// Processing Element (PE) Information
//

struct ProcessingElement {
  uint32_t pe_id;           // Processing element ID (0 to num_pes-1)
  uint32_t physical_cpu;    // Physical CPU number
  uint64_t local_memory;    // Local memory size in bytes
  uint64_t global_memory;   // Accessible global memory
  bool is_active;           // PE is active
  bool is_service_pe;       // True if service PE, false if compute PE
  char hostname[256];       // PE hostname (for distributed debugging)
};

//
// System Configuration
//

struct SystemConfiguration {
  uint32_t num_pes;         // Total number of processing elements
  uint32_t num_service_pes; // Number of service PEs
  uint32_t num_compute_pes; // Number of compute PEs
  uint64_t total_memory;    // Total system memory in bytes
  uint64_t global_memory;   // Global shared memory
  char system_type[64];     // "T3D", "T3E", "SV1", "X1", etc.
  uint32_t interconnect_type; // Interconnect topology type
};

//
// Parallel Process Management
//

enum class ParallelProcessState {
  CREATED,      // Process created but not started
  RUNNING,      // Process is running
  STOPPED,      // Process stopped (by debugger)
  BARRIER,      // Process waiting at barrier
  SLEEPING,     // Process sleeping
  ZOMBIE,       // Process terminated but not reaped
  DISTRIBUTED,  // Process distributed across PEs
};

// Process distribution information
struct ProcessDistribution {
  ProcessId pid;            // Process ID
  uint32_t num_pes_used;    // Number of PEs this process uses
  uint32_t pe_list[2176];   // List of PE IDs (max T3E size)
  uint64_t memory_per_pe;   // Memory allocated per PE
  bool is_replicated;       // True if replicated, false if partitioned
};

//
// NUMA Memory Management
//

enum class MemoryLocation {
  LOCAL,        // Local PE memory (fast access)
  REMOTE,       // Remote PE memory (slower access)
  GLOBAL,       // Global shared memory
  E_REGISTER,   // E-registers (T3E special distributed memory)
};

struct MemorySegment {
  uint64_t virtual_address;     // Virtual address
  uint64_t physical_address;    // Physical address
  uint64_t size;                // Segment size in bytes
  MemoryLocation location;      // Memory location type
  uint32_t home_pe;             // Home PE for this memory
  uint32_t permissions;         // Access permissions
  bool is_distributed;          // Distributed across PEs
  uint32_t distribution_stride; // Stride for distributed memory
};

//
// Barrier Synchronization
//

struct BarrierInfo {
  uint64_t barrier_id;      // Barrier identifier
  uint32_t num_waiting;     // Number of PEs waiting
  uint32_t num_expected;    // Expected number of PEs
  uint32_t pe_list[2176];   // List of PEs at barrier
  bool is_hierarchical;     // Hierarchical barrier
};

//
// Message Passing (MPI Support)
//

struct MPIInfo {
  uint32_t rank;            // MPI rank
  uint32_t world_size;      // MPI world size
  uint32_t communicator_id; // Communicator ID
  uint64_t messages_sent;   // Messages sent
  uint64_t messages_received; // Messages received
  uint64_t bytes_sent;      // Bytes sent
  uint64_t bytes_received;  // Bytes received
};

//
// T3D/T3E Specific Features
//

// E-registers (T3E distributed memory registers)
struct ERegisters {
  // E-registers provide fast distributed shared memory
  // 8 E-registers per PE, each can map to remote PE memory
  uint64_t e_reg[8];        // E-register values
  uint32_t e_pe[8];         // Target PE for each E-register
  uint64_t e_addr[8];       // Remote address for each E-register
  bool e_valid[8];          // E-register valid bits
};

// 3D torus interconnect (T3E)
struct TorusCoordinates {
  uint32_t x;               // X coordinate in torus
  uint32_t y;               // Y coordinate in torus
  uint32_t z;               // Z coordinate in torus
};

//
// UNICOS/mk Debugging Interface
//

class Debug {
public:
  //
  // System-wide operations
  //

  static ErrorCode getSystemConfiguration(SystemConfiguration &config);
  static ErrorCode enumerateProcessingElements(std::vector<ProcessingElement> &pes);
  static ErrorCode getProcessingElement(uint32_t pe_id, ProcessingElement &pe);

  //
  // Process control (parallel-aware)
  //

  static ErrorCode attach(ProcessId pid);
  static ErrorCode attachOnPE(ProcessId pid, uint32_t pe_id);  // Attach to specific PE
  static ErrorCode detach(ProcessId pid);
  static ErrorCode suspend(ProcessId pid);
  static ErrorCode suspendOnPE(ProcessId pid, uint32_t pe_id); // Suspend on specific PE
  static ErrorCode resume(ProcessId pid);
  static ErrorCode resumeOnPE(ProcessId pid, uint32_t pe_id);
  static ErrorCode terminate(ProcessId pid, int signal);

  //
  // Process distribution
  //

  static ErrorCode getProcessDistribution(ProcessId pid, ProcessDistribution &dist);
  static ErrorCode getProcessState(ProcessId pid, uint32_t pe_id, ParallelProcessState &state);

  //
  // Memory operations (NUMA-aware)
  //

  static ErrorCode readMemory(ProcessId pid, uint32_t pe_id, uint64_t address,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(ProcessId pid, uint32_t pe_id, uint64_t address,
                               const void *buffer, size_t length);

  // Global memory operations
  static ErrorCode readGlobalMemory(uint64_t global_address, void *buffer, size_t length);
  static ErrorCode writeGlobalMemory(uint64_t global_address, const void *buffer, size_t length);

  // Enumerate memory segments
  static ErrorCode enumerateMemorySegments(ProcessId pid, uint32_t pe_id,
                                           std::vector<MemorySegment> &segments);

  //
  // Register operations (per PE)
  //

  // For T3D/T3E: uses DEC Alpha CPU state
  static ErrorCode readCPUState(ProcessId pid, uint32_t pe_id,
                                Architecture::Alpha::CPUState &state);
  static ErrorCode writeCPUState(ProcessId pid, uint32_t pe_id,
                                 const Architecture::Alpha::CPUState &state);

  // For SV1: uses Cray vector CPU state
  static ErrorCode readCrayCPUState(ProcessId pid, uint32_t pe_id,
                                    Architecture::Cray::CPUState &state);
  static ErrorCode writeCrayCPUState(ProcessId pid, uint32_t pe_id,
                                     const Architecture::Cray::CPUState &state);

  //
  // T3E E-registers
  //

  static ErrorCode readERegisters(ProcessId pid, uint32_t pe_id, ERegisters &eregs);
  static ErrorCode writeERegisters(ProcessId pid, uint32_t pe_id, const ERegisters &eregs);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(ProcessId pid, uint32_t pe_id, uint64_t address);
  static ErrorCode setBreakpointAllPEs(ProcessId pid, uint64_t address); // All PEs
  static ErrorCode removeBreakpoint(ProcessId pid, uint32_t pe_id, uint64_t address);
  static ErrorCode removeBreakpointAllPEs(ProcessId pid, uint64_t address);

  //
  // Barrier debugging
  //

  static ErrorCode enumerateBarriers(std::vector<BarrierInfo> &barriers);
  static ErrorCode getBarrierInfo(uint64_t barrier_id, BarrierInfo &info);
  static ErrorCode breakOnBarrier(uint64_t barrier_id);  // Break when barrier reached

  //
  // MPI debugging support
  //

  static ErrorCode getMPIInfo(ProcessId pid, uint32_t pe_id, MPIInfo &info);
  static ErrorCode interceptMPICall(ProcessId pid, const char *mpi_function);

  //
  // Performance monitoring
  //

  struct PerformanceCounters {
    uint64_t cpu_cycles;          // CPU cycles
    uint64_t instructions;        // Instructions executed
    uint64_t local_memory_refs;   // Local memory references
    uint64_t remote_memory_refs;  // Remote memory references
    uint64_t e_register_refs;     // E-register references (T3E)
    uint64_t network_messages;    // Network messages sent/received
    double bandwidth_local;       // Local memory bandwidth (GB/s)
    double bandwidth_remote;      // Remote memory bandwidth (GB/s)
    double bandwidth_network;     // Network bandwidth (GB/s)
  };

  static ErrorCode getPerformanceCounters(ProcessId pid, uint32_t pe_id,
                                          PerformanceCounters &counters);

  //
  // Topology information
  //

  static ErrorCode getTorusCoordinates(uint32_t pe_id, TorusCoordinates &coords);
  static ErrorCode getPEFromCoordinates(const TorusCoordinates &coords, uint32_t &pe_id);

  //
  // Distributed debugging coordination
  //

  struct DebugSession {
    ProcessId pid;
    uint32_t num_pes;
    uint32_t active_pes;          // Number of PEs currently stopped
    uint32_t stopped_pe_list[2176]; // List of stopped PEs
  };

  static ErrorCode createDebugSession(ProcessId pid, DebugSession &session);
  static ErrorCode synchronizeAllPEs(const DebugSession &session); // Stop all PEs
  static ErrorCode stepAllPEs(const DebugSession &session);        // Single-step all PEs
};

//
// UNICOS/mk System Calls
//

namespace SystemCalls {
  // Standard Unix system calls (inherited from Mach)
  constexpr int UNICOSmk_exit      = 1;
  constexpr int UNICOSmk_fork      = 2;
  constexpr int UNICOSmk_read      = 3;
  constexpr int UNICOSmk_write     = 4;
  constexpr int UNICOSmk_open      = 5;
  constexpr int UNICOSmk_close     = 6;

  // UNICOS/mk parallel extensions
  constexpr int UNICOSmk_pe_info       = 200;  // Get PE information
  constexpr int UNICOSmk_global_malloc = 201;  // Allocate global memory
  constexpr int UNICOSmk_global_free   = 202;  // Free global memory
  constexpr int UNICOSmk_barrier       = 203;  // Barrier synchronization
  constexpr int UNICOSmk_shmem         = 204;  // Shared memory operations
  constexpr int UNICOSmk_e_register    = 205;  // E-register operations (T3E)
}

//
// UNICOS/mk Signals (Mach-based)
//

namespace Signals {
  constexpr int SIGHUP     = 1;
  constexpr int SIGINT     = 2;
  constexpr int SIGQUIT    = 3;
  constexpr int SIGILL     = 4;
  constexpr int SIGTRAP    = 5;
  constexpr int SIGABRT    = 6;
  constexpr int SIGEMT     = 7;
  constexpr int SIGFPE     = 8;
  constexpr int SIGKILL    = 9;
  constexpr int SIGBUS     = 10;
  constexpr int SIGSEGV    = 11;
  constexpr int SIGSYS     = 12;
  constexpr int SIGPIPE    = 13;
  constexpr int SIGALRM    = 14;
  constexpr int SIGTERM    = 15;
  constexpr int SIGURG     = 16;
  constexpr int SIGSTOP    = 17;
  constexpr int SIGTSTP    = 18;
  constexpr int SIGCONT    = 19;
  constexpr int SIGCHLD    = 20;

  // UNICOS/mk parallel signals
  constexpr int SIGBARRIER = 50;  // Barrier reached
  constexpr int SIGDIST    = 51;  // Distributed operation complete
}

} // namespace UNICOSmk
} // namespace Host
} // namespace ds2
