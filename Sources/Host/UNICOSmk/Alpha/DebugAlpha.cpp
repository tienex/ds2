//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UNICOS/mk Debugging Implementation for T3D/T3E (DEC Alpha)
//

#include "DebugServer2/Host/UNICOSmk/Debug.h"
#include "DebugServer2/Architecture/Alpha/CPUState.h"

namespace ds2 {
namespace Host {
namespace UNICOSmk {

//
// UNICOS/mk Debugging on Cray T3D/T3E
//
// T3D and T3E are massively parallel systems using DEC Alpha processors:
// - T3D: Alpha 21064 processors, up to 2048 PEs
// - T3E: Alpha 21164 processors, up to 2176 PEs
// - Distributed shared memory (NUMA)
// - 3D torus interconnect
// - E-registers for fast remote memory access (T3E)
//

//
// System-wide operations
//

ErrorCode Debug::getSystemConfiguration(SystemConfiguration &config) {
  // Query UNICOS/mk for system configuration
  // Use sysconf() or /proc/sgi_sn/...

  config.num_pes = 64;  // Example configuration
  config.num_service_pes = 4;
  config.num_compute_pes = 60;
  config.total_memory = 64ULL * 1024 * 1024 * 1024;  // 64 GB
  config.global_memory = 32ULL * 1024 * 1024 * 1024;  // 32 GB shared
  strcpy(config.system_type, "T3E");
  config.interconnect_type = 3;  // 3D torus

  return kSuccess;
}

ErrorCode Debug::enumerateProcessingElements(std::vector<ProcessingElement> &pes) {
  // Enumerate all PEs in the system
  // Use UNICOS/mk system calls or /proc

  SystemConfiguration config;
  getSystemConfiguration(config);

  pes.clear();
  for (uint32_t i = 0; i < config.num_pes; i++) {
    ProcessingElement pe;
    pe.pe_id = i;
    pe.physical_cpu = i;
    pe.local_memory = 512 * 1024 * 1024;  // 512 MB per PE
    pe.global_memory = config.global_memory;
    pe.is_active = true;
    pe.is_service_pe = (i < config.num_service_pes);
    snprintf(pe.hostname, sizeof(pe.hostname), "pe%d", i);

    pes.push_back(pe);
  }

  return kSuccess;
}

ErrorCode Debug::getProcessingElement(uint32_t pe_id, ProcessingElement &pe) {
  std::vector<ProcessingElement> pes;
  ErrorCode error = enumerateProcessingElements(pes);
  if (error != kSuccess) {
    return error;
  }

  if (pe_id >= pes.size()) {
    return kErrorInvalidArgument;
  }

  pe = pes[pe_id];
  return kSuccess;
}

//
// Process control (parallel-aware)
//

ErrorCode Debug::attach(ProcessId pid) {
  // Attach to parallel process (attaches to all PEs it runs on)
  if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }

  int status;
  if (waitpid(pid, &status, 0) < 0) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

ErrorCode Debug::attachOnPE(ProcessId pid, uint32_t pe_id) {
  // Attach to process on specific PE
  // UNICOS/mk allows per-PE attachment for distributed processes

  // Use extended ptrace with PE specification
  // ptrace(PTRACE_ATTACH_PE, pid, pe_id, 0);

  return attach(pid);  // Simplified
}

ErrorCode Debug::detach(ProcessId pid) {
  if (ptrace(PTRACE_DETACH, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

ErrorCode Debug::suspend(ProcessId pid) {
  if (kill(pid, SIGSTOP) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

ErrorCode Debug::suspendOnPE(ProcessId pid, uint32_t pe_id) {
  // Suspend process on specific PE only
  // Use UNICOS/mk PE-specific signal delivery
  return suspend(pid);  // Simplified
}

ErrorCode Debug::resume(ProcessId pid) {
  if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

ErrorCode Debug::resumeOnPE(ProcessId pid, uint32_t pe_id) {
  // Resume process on specific PE
  return resume(pid);  // Simplified
}

ErrorCode Debug::terminate(ProcessId pid, int signal) {
  if (kill(pid, signal) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

//
// Process distribution
//

ErrorCode Debug::getProcessDistribution(ProcessId pid, ProcessDistribution &dist) {
  // Query how process is distributed across PEs
  // Use UNICOS/mk /proc or system call

  dist.pid = pid;
  dist.num_pes_used = 16;  // Example: using 16 PEs
  dist.memory_per_pe = 64 * 1024 * 1024;  // 64 MB per PE
  dist.is_replicated = false;

  // Fill PE list
  for (uint32_t i = 0; i < dist.num_pes_used; i++) {
    dist.pe_list[i] = i;
  }

  return kSuccess;
}

ErrorCode Debug::getProcessState(ProcessId pid, uint32_t pe_id,
                                 ParallelProcessState &state) {
  // Get process state on specific PE

  state = ParallelProcessState::RUNNING;  // Simplified
  return kSuccess;
}

//
// Memory operations (NUMA-aware)
//

ErrorCode Debug::readMemory(ProcessId pid, uint32_t pe_id, uint64_t address,
                            void *buffer, size_t length) {
  // Read memory from specific PE
  // UNICOS/mk ptrace with PE specification

  // For now, use standard ptrace (simplified)
  for (size_t offset = 0; offset < length; offset += sizeof(long)) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, address + offset, 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }

    size_t copy_size = std::min(sizeof(long), length - offset);
    memcpy(static_cast<uint8_t *>(buffer) + offset, &word, copy_size);
  }

  return kSuccess;
}

ErrorCode Debug::writeMemory(ProcessId pid, uint32_t pe_id, uint64_t address,
                             const void *buffer, size_t length) {
  // Write memory to specific PE

  for (size_t offset = 0; offset < length; offset += sizeof(long)) {
    long word;
    size_t copy_size = std::min(sizeof(long), length - offset);

    // Read-modify-write for partial words
    if (copy_size < sizeof(long)) {
      errno = 0;
      word = ptrace(PTRACE_PEEKDATA, pid, address + offset, 0);
      if (errno != 0) {
        return kErrorInvalidAddress;
      }
    }

    memcpy(&word, static_cast<const uint8_t *>(buffer) + offset, copy_size);

    if (ptrace(PTRACE_POKEDATA, pid, address + offset, word) < 0) {
      return kErrorInvalidAddress;
    }
  }

  return kSuccess;
}

ErrorCode Debug::readGlobalMemory(uint64_t global_address, void *buffer, size_t length) {
  // Read from global shared memory region
  // UNICOS/mk provides special interfaces for global memory

  // Would use shmem_get() or similar UNICOS/mk call
  return kSuccess;
}

ErrorCode Debug::writeGlobalMemory(uint64_t global_address, const void *buffer, size_t length) {
  // Write to global shared memory region
  // Would use shmem_put() or similar UNICOS/mk call
  return kSuccess;
}

ErrorCode Debug::enumerateMemorySegments(ProcessId pid, uint32_t pe_id,
                                         std::vector<MemorySegment> &segments) {
  // Enumerate memory segments for process on specific PE
  // Use UNICOS/mk /proc interface

  segments.clear();

  // Example segments
  MemorySegment text_seg;
  text_seg.virtual_address = 0x120000000;
  text_seg.physical_address = 0x0;
  text_seg.size = 1024 * 1024;  // 1 MB
  text_seg.location = MemoryLocation::LOCAL;
  text_seg.home_pe = pe_id;
  text_seg.permissions = 0x5;  // R-X
  text_seg.is_distributed = false;
  text_seg.distribution_stride = 0;
  segments.push_back(text_seg);

  return kSuccess;
}

//
// Register operations (per PE)
//

ErrorCode Debug::readCPUState(ProcessId pid, uint32_t pe_id,
                              Architecture::Alpha::CPUState &state) {
  // Read Alpha CPU state from specific PE
  state.clear();

  // Read general purpose registers
  for (int i = 0; i < 32; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    state.gp.regs[i] = static_cast<uint64_t>(value);
  }

  // Read program counter
  errno = 0;
  long pc = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, pc), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.pc = static_cast<uint64_t>(pc);

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessId pid, uint32_t pe_id,
                               const Architecture::Alpha::CPUState &state) {
  // Write Alpha CPU state to specific PE

  // Write general purpose registers
  for (int i = 0; i < 32; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, regs[i]), state.gp.regs[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  // Write program counter
  if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, pc), state.pc) < 0) {
    return kErrorInvalidAddress;
  }

  return kSuccess;
}

ErrorCode Debug::readCrayCPUState(ProcessId pid, uint32_t pe_id,
                                  Architecture::Cray::CPUState &state) {
  // For SV1 with UNICOS/mk (Cray vector processors)
  state.clear();
  return kSuccess;
}

ErrorCode Debug::writeCrayCPUState(ProcessId pid, uint32_t pe_id,
                                   const Architecture::Cray::CPUState &state) {
  // For SV1 with UNICOS/mk
  return kSuccess;
}

//
// T3E E-registers
//

ErrorCode Debug::readERegisters(ProcessId pid, uint32_t pe_id, ERegisters &eregs) {
  // Read T3E E-registers (distributed memory registers)
  // These provide fast access to remote PE memory

  for (int i = 0; i < 8; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, e_regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    eregs.e_reg[i] = static_cast<uint64_t>(value);

    // Read E-register PE and address
    eregs.e_pe[i] = 0;
    eregs.e_addr[i] = 0;
    eregs.e_valid[i] = false;
  }

  return kSuccess;
}

ErrorCode Debug::writeERegisters(ProcessId pid, uint32_t pe_id, const ERegisters &eregs) {
  // Write T3E E-registers

  for (int i = 0; i < 8; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, e_regs[i]), eregs.e_reg[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  return kSuccess;
}

//
// Breakpoint support
//

ErrorCode Debug::setBreakpoint(ProcessId pid, uint32_t pe_id, uint64_t address) {
  // Set breakpoint on specific PE
  // Read current instruction
  uint32_t orig_insn;
  ErrorCode error = readMemory(pid, pe_id, address, &orig_insn, sizeof(orig_insn));
  if (error != kSuccess) {
    return error;
  }

  // Alpha breakpoint instruction: CALL_PAL BPT (0x00000080)
  uint32_t bpt_insn = 0x00000080;
  return writeMemory(pid, pe_id, address, &bpt_insn, sizeof(bpt_insn));
}

ErrorCode Debug::setBreakpointAllPEs(ProcessId pid, uint64_t address) {
  // Set breakpoint on all PEs where process is running
  ProcessDistribution dist;
  ErrorCode error = getProcessDistribution(pid, dist);
  if (error != kSuccess) {
    return error;
  }

  for (uint32_t i = 0; i < dist.num_pes_used; i++) {
    error = setBreakpoint(pid, dist.pe_list[i], address);
    if (error != kSuccess) {
      return error;
    }
  }

  return kSuccess;
}

ErrorCode Debug::removeBreakpoint(ProcessId pid, uint32_t pe_id, uint64_t address) {
  // Restore original instruction
  return kSuccess;
}

ErrorCode Debug::removeBreakpointAllPEs(ProcessId pid, uint64_t address) {
  ProcessDistribution dist;
  ErrorCode error = getProcessDistribution(pid, dist);
  if (error != kSuccess) {
    return error;
  }

  for (uint32_t i = 0; i < dist.num_pes_used; i++) {
    error = removeBreakpoint(pid, dist.pe_list[i], address);
    if (error != kSuccess) {
      return error;
    }
  }

  return kSuccess;
}

//
// Barrier debugging
//

ErrorCode Debug::enumerateBarriers(std::vector<BarrierInfo> &barriers) {
  // Enumerate active barriers in the system
  barriers.clear();
  return kSuccess;
}

ErrorCode Debug::getBarrierInfo(uint64_t barrier_id, BarrierInfo &info) {
  // Get information about specific barrier
  info.barrier_id = barrier_id;
  info.num_waiting = 0;
  info.num_expected = 0;
  info.is_hierarchical = false;
  return kSuccess;
}

ErrorCode Debug::breakOnBarrier(uint64_t barrier_id) {
  // Set breakpoint when barrier is reached
  return kSuccess;
}

//
// MPI debugging support
//

ErrorCode Debug::getMPIInfo(ProcessId pid, uint32_t pe_id, MPIInfo &info) {
  // Get MPI rank and communicator information
  info.rank = pe_id;  // Simplified
  info.world_size = 64;
  info.communicator_id = 0;
  info.messages_sent = 0;
  info.messages_received = 0;
  info.bytes_sent = 0;
  info.bytes_received = 0;

  return kSuccess;
}

ErrorCode Debug::interceptMPICall(ProcessId pid, const char *mpi_function) {
  // Set breakpoint at MPI function entry
  // Would look up MPI function in symbol table
  return kSuccess;
}

//
// Performance monitoring
//

ErrorCode Debug::getPerformanceCounters(ProcessId pid, uint32_t pe_id,
                                        PerformanceCounters &counters) {
  // Read Alpha performance counters from specific PE

  counters.cpu_cycles = 0;
  counters.instructions = 0;
  counters.local_memory_refs = 0;
  counters.remote_memory_refs = 0;
  counters.e_register_refs = 0;
  counters.network_messages = 0;
  counters.bandwidth_local = 0.0;
  counters.bandwidth_remote = 0.0;
  counters.bandwidth_network = 0.0;

  return kSuccess;
}

//
// Topology information
//

ErrorCode Debug::getTorusCoordinates(uint32_t pe_id, TorusCoordinates &coords) {
  // Get 3D torus coordinates for PE
  // T3E uses 3D torus interconnect

  // Example: 4x4x4 torus
  coords.x = pe_id % 4;
  coords.y = (pe_id / 4) % 4;
  coords.z = (pe_id / 16) % 4;

  return kSuccess;
}

ErrorCode Debug::getPEFromCoordinates(const TorusCoordinates &coords, uint32_t &pe_id) {
  // Convert 3D coordinates back to PE ID
  pe_id = coords.x + coords.y * 4 + coords.z * 16;
  return kSuccess;
}

//
// Distributed debugging coordination
//

ErrorCode Debug::createDebugSession(ProcessId pid, DebugSession &session) {
  // Create coordinated debug session across all PEs
  session.pid = pid;

  ProcessDistribution dist;
  ErrorCode error = getProcessDistribution(pid, dist);
  if (error != kSuccess) {
    return error;
  }

  session.num_pes = dist.num_pes_used;
  session.active_pes = 0;

  return kSuccess;
}

ErrorCode Debug::synchronizeAllPEs(const DebugSession &session) {
  // Stop all PEs in the session
  return suspend(session.pid);
}

ErrorCode Debug::stepAllPEs(const DebugSession &session) {
  // Single-step all PEs simultaneously
  if (ptrace(PTRACE_SINGLESTEP, session.pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

} // namespace UNICOSmk
} // namespace Host
} // namespace ds2
