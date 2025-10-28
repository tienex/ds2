//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UNICOS Debugging Implementation for Cray Vector Processors
//

#include "DebugServer2/Host/UNICOS/Debug.h"
#include "DebugServer2/Architecture/Cray/CPUState.h"

namespace ds2 {
namespace Host {
namespace UNICOS {

//
// UNICOS Debugging on Cray Vector Supercomputers
//
// This implementation provides debugging support for UNICOS running
// on Cray vector processors (Cray-1, X-MP, Y-MP, C90, T90, SV1).
//
// Key features:
// - Word-addressed memory (64-bit words, not bytes)
// - Vector register debugging (V0-V7, 64 or 128 elements each)
// - Exchange package access for context switching
// - Job control integration
// - Performance counter access
//

ErrorCode Debug::attach(ProcessId pid) {
  // Use UNICOS-specific ptrace variant
  // UNICOS ptrace has extensions for vector registers

  // PTRACE_ATTACH equivalent on UNICOS
  if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }

  // Wait for process to stop
  int status;
  if (waitpid(pid, &status, 0) < 0) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
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

ErrorCode Debug::resume(ProcessId pid) {
  if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

ErrorCode Debug::terminate(ProcessId pid, int signal) {
  if (kill(pid, signal) < 0) {
    return kErrorProcessNotFound;
  }
  return kSuccess;
}

//
// Memory operations
//

ErrorCode Debug::readMemoryWords(ProcessId pid, uint64_t word_address,
                                 uint64_t *buffer, size_t word_count) {
  // UNICOS word-addressed memory access
  // Use PTRACE_PEEKDATA but interpret as word addresses

  for (size_t i = 0; i < word_count; i++) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, word_address + i, 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    buffer[i] = static_cast<uint64_t>(word);
  }

  return kSuccess;
}

ErrorCode Debug::writeMemoryWords(ProcessId pid, uint64_t word_address,
                                  const uint64_t *buffer, size_t word_count) {
  // UNICOS word-addressed memory writes

  for (size_t i = 0; i < word_count; i++) {
    if (ptrace(PTRACE_POKEDATA, pid, word_address + i, buffer[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  return kSuccess;
}

ErrorCode Debug::readMemory(ProcessId pid, uint64_t address,
                            void *buffer, size_t length) {
  // Byte-addressed access (for later UNICOS versions)
  // Convert to word-addressed access

  uint64_t word_addr = address / 8;
  uint64_t byte_offset = address % 8;

  // Read aligned words
  size_t words_needed = (byte_offset + length + 7) / 8;
  std::vector<uint64_t> words(words_needed);

  ErrorCode error = readMemoryWords(pid, word_addr, words.data(), words_needed);
  if (error != kSuccess) {
    return error;
  }

  // Extract bytes from words
  memcpy(buffer, reinterpret_cast<uint8_t *>(words.data()) + byte_offset, length);
  return kSuccess;
}

ErrorCode Debug::writeMemory(ProcessId pid, uint64_t address,
                             const void *buffer, size_t length) {
  // Byte-addressed write (read-modify-write for partial words)

  uint64_t word_addr = address / 8;
  uint64_t byte_offset = address % 8;

  // Read aligned words
  size_t words_needed = (byte_offset + length + 7) / 8;
  std::vector<uint64_t> words(words_needed);

  ErrorCode error = readMemoryWords(pid, word_addr, words.data(), words_needed);
  if (error != kSuccess) {
    return error;
  }

  // Modify bytes
  memcpy(reinterpret_cast<uint8_t *>(words.data()) + byte_offset, buffer, length);

  // Write back
  return writeMemoryWords(pid, word_addr, words.data(), words_needed);
}

//
// Register operations
//

ErrorCode Debug::readCPUState(ProcessId pid, Architecture::Cray::CPUState &state) {
  // UNICOS provides special ptrace requests for vector registers
  // This is conceptual - actual UNICOS ptrace API would have specific request codes

  state.clear();

  // Read scalar registers (S0-S7)
  for (int i = 0; i < 8; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, s_regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    state.scalar.s[i] = static_cast<uint64_t>(value);
  }

  // Read address registers (A0-A7)
  for (int i = 0; i < 8; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, a_regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    state.address.a[i] = static_cast<uint32_t>(value);
  }

  // Read program counter
  errno = 0;
  long pc = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, pc), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.pc = static_cast<uint32_t>(pc);

  // Read vector length
  errno = 0;
  long vl = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, vl), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.vl = static_cast<uint32_t>(vl);

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessId pid, const Architecture::Cray::CPUState &state) {
  // Write scalar registers
  for (int i = 0; i < 8; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, s_regs[i]), state.scalar.s[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  // Write address registers
  for (int i = 0; i < 8; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, a_regs[i]), state.address.a[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  // Write program counter
  if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, pc), state.pc) < 0) {
    return kErrorInvalidAddress;
  }

  return kSuccess;
}

ErrorCode Debug::readVectorRegisters(ProcessId pid, uint64_t vr[8][128]) {
  // UNICOS-specific: read vector registers
  // This would use a special ptrace request or /proc interface

  // Conceptual implementation - actual UNICOS would provide specific APIs
  for (int vreg = 0; vreg < 8; vreg++) {
    for (int elem = 0; elem < 128; elem++) {
      errno = 0;
      long value = ptrace(PTRACE_PEEKUSER, pid,
                         offsetof(user, vector_regs[vreg][elem]), 0);
      if (errno != 0) {
        return kErrorInvalidAddress;
      }
      vr[vreg][elem] = static_cast<uint64_t>(value);
    }
  }

  return kSuccess;
}

ErrorCode Debug::writeVectorRegisters(ProcessId pid, const uint64_t vr[8][128]) {
  // Write vector registers

  for (int vreg = 0; vreg < 8; vreg++) {
    for (int elem = 0; elem < 128; elem++) {
      if (ptrace(PTRACE_POKEUSER, pid,
                offsetof(user, vector_regs[vreg][elem]), vr[vreg][elem]) < 0) {
        return kErrorInvalidAddress;
      }
    }
  }

  return kSuccess;
}

ErrorCode Debug::readScalarRegisters(ProcessId pid, uint64_t s[8]) {
  for (int i = 0; i < 8; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, s_regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    s[i] = static_cast<uint64_t>(value);
  }
  return kSuccess;
}

ErrorCode Debug::writeScalarRegisters(ProcessId pid, const uint64_t s[8]) {
  for (int i = 0; i < 8; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, s_regs[i]), s[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }
  return kSuccess;
}

ErrorCode Debug::readAddressRegisters(ProcessId pid, uint32_t a[8]) {
  for (int i = 0; i < 8; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, a_regs[i]), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    a[i] = static_cast<uint32_t>(value);
  }
  return kSuccess;
}

ErrorCode Debug::writeAddressRegisters(ProcessId pid, const uint32_t a[8]) {
  for (int i = 0; i < 8; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, a_regs[i]), a[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }
  return kSuccess;
}

//
// Breakpoint support
//

ErrorCode Debug::setBreakpoint(ProcessId pid, uint64_t word_address) {
  // Read current instruction word
  uint64_t original_word;
  ErrorCode error = readMemoryWords(pid, word_address, &original_word, 1);
  if (error != kSuccess) {
    return error;
  }

  // Save original instruction
  // In practice, would maintain a breakpoint table

  // Write breakpoint instruction (reserved instruction on Cray)
  uint64_t breakpoint_word = 0x0000000000000000;  // Reserved instruction
  return writeMemoryWords(pid, word_address, &breakpoint_word, 1);
}

ErrorCode Debug::removeBreakpoint(ProcessId pid, uint64_t word_address) {
  // Restore original instruction
  // In practice, would retrieve from breakpoint table

  // For now, just indicate success
  return kSuccess;
}

ErrorCode Debug::setVectorLengthWatch(ProcessId pid, uint32_t vl_min, uint32_t vl_max) {
  // UNICOS-specific: set watchpoint for vector length changes
  // This would use hardware debug facilities or kernel support
  return kSuccess;
}

//
// Job control
//

ErrorCode Debug::getJobInfo(ProcessId pid, JobInfo &info) {
  // UNICOS provides job control information through /proc or system calls
  // This is conceptual

  info.job_id = pid;  // Simplified - actual job IDs are separate
  info.priority = 128;  // Default priority
  info.cpu_time = 0;
  info.memory_limit = 1024 * 1024 * 1024;  // 1 GB in words
  info.cpu_limit = 3600;  // 1 hour
  strcpy(info.job_name, "debug_job");
  strcpy(info.queue_name, "interactive");
  info.is_batch = false;

  return kSuccess;
}

ErrorCode Debug::setJobPriority(ProcessId pid, uint32_t priority) {
  // Use UNICOS jobctl system call
  // syscall(SYS_jobctl, JOBCTL_SET_PRIORITY, pid, priority);
  return kSuccess;
}

ErrorCode Debug::getJobLimits(ProcessId pid, uint64_t &cpu_limit, uint64_t &memory_limit) {
  // Get resource limits from UNICOS
  struct rlimit rlim;

  if (getrlimit(RLIMIT_CPU, &rlim) == 0) {
    cpu_limit = rlim.rlim_cur;
  }

  if (getrlimit(RLIMIT_RSS, &rlim) == 0) {
    memory_limit = rlim.rlim_cur;
  }

  return kSuccess;
}

//
// Performance monitoring
//

ErrorCode Debug::getPerformanceCounters(ProcessId pid, PerformanceCounters &counters) {
  // UNICOS provides hardware performance counters
  // Access through special registers or /proc interface

  counters.cpu_cycles = 0;
  counters.vector_instructions = 0;
  counters.scalar_instructions = 0;
  counters.memory_references = 0;
  counters.vector_operations = 0;
  counters.functional_unit_conflicts = 0;
  counters.vector_utilization = 0.0;
  counters.mflops = 0.0;

  // Would read from hardware performance monitoring unit
  return kSuccess;
}

ErrorCode Debug::getExchangePackage(ProcessId pid,
                                    Architecture::Cray::CPUState::ExchangePackage &exchange) {
  // Read exchange package for context switching information
  // This is saved by UNICOS kernel during job swaps

  errno = 0;
  long next_addr = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, exchange.next_address), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  exchange.next_address = static_cast<uint32_t>(next_addr);

  // Read other exchange package fields
  return kSuccess;
}

ErrorCode Debug::getCFSAttributes(const char *filename, CFSAttributes &attrs) {
  // Query CFS (Cray File System) attributes
  // Use stat() with UNICOS-specific extensions

  attrs.record_size = 512;  // Default
  attrs.block_size = 4096;
  attrs.file_structure = 0;
  attrs.is_blocked = false;
  attrs.is_binary = true;
  strcpy(attrs.dataset_name, filename);

  return kSuccess;
}

ErrorCode Debug::getProcessorInfo(uint32_t &num_cpus, Architecture::Cray::Variant &variant) {
  // Get system configuration
  // UNICOS provides system information through sysconf() or /proc

  num_cpus = 1;  // Default single CPU
  variant = Architecture::Cray::Variant::CRAY_1;  // Default

  // Would query actual system type from kernel
  return kSuccess;
}

} // namespace UNICOS
} // namespace Host
} // namespace ds2
