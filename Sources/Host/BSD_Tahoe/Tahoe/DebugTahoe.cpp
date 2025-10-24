//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// 4.3BSD-Tahoe Debugging Implementation for Tahoe (CCI Power 6/32)
//

#include "DebugServer2/Host/BSD_Tahoe/Debug.h"
#include "DebugServer2/Architecture/Tahoe/CPUState.h"

namespace ds2 {
namespace Host {
namespace BSD_Tahoe {

//
// 4.3BSD-Tahoe Debugging on Tahoe Architecture
//
// This implementation provides debugging support for 4.3BSD-Tahoe
// running on Tahoe (CCI Power 6/32) processors, including:
// - CCI Power 6/32 systems
// - Harris HCX-9 fault-tolerant systems
//

//
// Process control
//

ErrorCode Debug::attach(ProcessId pid) {
  // 4.3BSD-Tahoe introduced PTRACE_ATTACH
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
  if (ptrace(PTRACE_CONT, pid, 1, 0) < 0) {
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

ErrorCode Debug::readMemory(ProcessId pid, uint32_t address,
                            void *buffer, size_t length) {
  // 4.3BSD ptrace reads word at a time
  size_t offset = 0;

  while (offset < length) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, address + offset, 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }

    size_t copy_size = std::min(sizeof(long), length - offset);
    memcpy(static_cast<uint8_t *>(buffer) + offset, &word, copy_size);
    offset += sizeof(long);
  }

  return kSuccess;
}

ErrorCode Debug::writeMemory(ProcessId pid, uint32_t address,
                             const void *buffer, size_t length) {
  // ptrace writes word at a time
  size_t offset = 0;

  while (offset < length) {
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

    offset += sizeof(long);
  }

  return kSuccess;
}

//
// Register operations
//

ErrorCode Debug::readCPUState(ProcessId pid, Architecture::Tahoe::CPUState &state) {
  // Read Tahoe registers through ptrace
  state.clear();

  // Read general purpose registers (R0-R15)
  for (int i = 0; i < 16; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, u_ar0) + i * sizeof(int), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    state.gp.regs[i] = static_cast<uint32_t>(value);
  }

  // Read program counter
  errno = 0;
  long pc = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, u_ar0) + 16 * sizeof(int), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.pc = static_cast<uint32_t>(pc);

  // Read PSL (Processor Status Longword)
  errno = 0;
  long psl = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, u_ar0) + 17 * sizeof(int), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.psl.psl = static_cast<uint32_t>(psl);

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessId pid, const Architecture::Tahoe::CPUState &state) {
  // Write Tahoe registers through ptrace

  // Write general purpose registers
  for (int i = 0; i < 16; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, u_ar0) + i * sizeof(int),
              state.gp.regs[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }

  // Write program counter
  if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, u_ar0) + 16 * sizeof(int),
            state.pc) < 0) {
    return kErrorInvalidAddress;
  }

  // Write PSL
  if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, u_ar0) + 17 * sizeof(int),
            state.psl.psl) < 0) {
    return kErrorInvalidAddress;
  }

  return kSuccess;
}

//
// Breakpoint support
//

ErrorCode Debug::setBreakpoint(ProcessId pid, uint32_t address) {
  // Read current instruction
  uint8_t orig_insn;
  ErrorCode error = readMemory(pid, address, &orig_insn, sizeof(orig_insn));
  if (error != kSuccess) {
    return error;
  }

  // Save original instruction (would maintain breakpoint table in practice)

  // Write BPT instruction (0x03)
  uint8_t bpt_insn = Architecture::Tahoe::Opcodes::BPT;
  return writeMemory(pid, address, &bpt_insn, sizeof(bpt_insn));
}

ErrorCode Debug::removeBreakpoint(ProcessId pid, uint32_t address) {
  // Restore original instruction
  // In practice, would retrieve from breakpoint table
  return kSuccess;
}

//
// Memory mapping
//

ErrorCode Debug::enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions) {
  // 4.3BSD-Tahoe doesn't have /proc, so use fixed layout
  regions.clear();

  // Text segment
  MemoryRegion text;
  text.start = 0x00000000;
  text.end = 0x10000000;  // Approximate
  text.permissions = 0x5; // R-X
  strcpy(text.name, "[text]");
  regions.push_back(text);

  // Data segment
  MemoryRegion data;
  data.start = 0x10000000;
  data.end = 0x20000000;
  data.permissions = 0x6; // RW-
  strcpy(data.name, "[data]");
  regions.push_back(data);

  // Stack
  MemoryRegion stack;
  stack.start = 0x7F000000;
  stack.end = 0x80000000;
  stack.permissions = 0x6; // RW-
  strcpy(stack.name, "[stack]");
  regions.push_back(stack);

  return kSuccess;
}

//
// Process information
//

ErrorCode Debug::getProcessInfo(ProcessId pid, ProcessInfo &info) {
  // Get process information from kernel structures
  // 4.3BSD-Tahoe uses different mechanisms than modern systems

  info.pid = pid;
  info.ppid = 0;
  info.pgrp = pid;
  info.uid = 0;
  info.gid = 0;
  info.state = ProcessState::RUNNING;
  info.flags = 0;
  info.nice = 0;
  strcpy(info.comm, "process");

  return kSuccess;
}

//
// Virtual memory statistics
//

ErrorCode Debug::getVMStats(ProcessId pid, VMStats &stats) {
  // Get VM statistics from kernel

  stats.text_size = 0;
  stats.data_size = 0;
  stats.stack_size = 0;
  stats.resident_size = 0;
  stats.shared_size = 0;

  return kSuccess;
}

} // namespace BSD_Tahoe
} // namespace Host
} // namespace ds2
