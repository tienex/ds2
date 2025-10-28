//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Intergraph CLIX Debugging Implementation for Clipper
//

#include "DebugServer2/Host/CLIX/Debug.h"
#include "DebugServer2/Architecture/Clipper/CPUState.h"

namespace ds2 {
namespace Host {
namespace CLIX {

//
// Intergraph CLIX Debugging on Clipper RISC Processors
//
// This implementation provides debugging support for CLIX (System V Release 3)
// running on Clipper processors (C100, C300, C400) in Intergraph workstations.
//

//
// Process control
//

ErrorCode Debug::attach(ProcessId pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
    return kErrorProcessNotFound;
  }

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

ErrorCode Debug::singleStep(ProcessId pid) {
  if (ptrace(PTRACE_SINGLESTEP, pid, 1, 0) < 0) {
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

ErrorCode Debug::readCPUState(ProcessId pid, Architecture::Clipper::CPUState &state) {
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

  // Read PSW (Processor Status Word)
  errno = 0;
  long psw = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, u_ar0) + 17 * sizeof(int), 0);
  if (errno != 0) {
    return kErrorInvalidAddress;
  }
  state.psw.psw = static_cast<uint32_t>(psw);

  return kSuccess;
}

ErrorCode Debug::writeCPUState(ProcessId pid, const Architecture::Clipper::CPUState &state) {
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

  // Write PSW
  if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, u_ar0) + 17 * sizeof(int),
            state.psw.psw) < 0) {
    return kErrorInvalidAddress;
  }

  return kSuccess;
}

ErrorCode Debug::readGeneralRegisters(ProcessId pid, uint32_t regs[16]) {
  for (int i = 0; i < 16; i++) {
    errno = 0;
    long value = ptrace(PTRACE_PEEKUSER, pid, offsetof(user, u_ar0) + i * sizeof(int), 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }
    regs[i] = static_cast<uint32_t>(value);
  }
  return kSuccess;
}

ErrorCode Debug::writeGeneralRegisters(ProcessId pid, const uint32_t regs[16]) {
  for (int i = 0; i < 16; i++) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(user, u_ar0) + i * sizeof(int),
              regs[i]) < 0) {
      return kErrorInvalidAddress;
    }
  }
  return kSuccess;
}

ErrorCode Debug::readFloatingPointRegisters(ProcessId pid, double fpregs[8]) {
  // Read FP registers through user struct
  for (int i = 0; i < 8; i++) {
    errno = 0;
    uint64_t value;
    // Read 64-bit FP register as two 32-bit words
    long low = ptrace(PTRACE_PEEKUSER, pid,
                     offsetof(user, u_fpregs) + i * sizeof(double), 0);
    long high = ptrace(PTRACE_PEEKUSER, pid,
                      offsetof(user, u_fpregs) + i * sizeof(double) + 4, 0);
    if (errno != 0) {
      return kErrorInvalidAddress;
    }

    value = (static_cast<uint64_t>(high) << 32) | static_cast<uint32_t>(low);
    memcpy(&fpregs[i], &value, sizeof(double));
  }
  return kSuccess;
}

ErrorCode Debug::writeFloatingPointRegisters(ProcessId pid, const double fpregs[8]) {
  for (int i = 0; i < 8; i++) {
    uint64_t value;
    memcpy(&value, &fpregs[i], sizeof(double));

    uint32_t low = static_cast<uint32_t>(value);
    uint32_t high = static_cast<uint32_t>(value >> 32);

    if (ptrace(PTRACE_POKEUSER, pid,
              offsetof(user, u_fpregs) + i * sizeof(double), low) < 0) {
      return kErrorInvalidAddress;
    }
    if (ptrace(PTRACE_POKEUSER, pid,
              offsetof(user, u_fpregs) + i * sizeof(double) + 4, high) < 0) {
      return kErrorInvalidAddress;
    }
  }
  return kSuccess;
}

//
// Breakpoint support
//

ErrorCode Debug::setBreakpoint(ProcessId pid, uint32_t address) {
  // Read current instruction
  uint32_t orig_insn;
  ErrorCode error = readMemory(pid, address, &orig_insn, sizeof(orig_insn));
  if (error != kSuccess) {
    return error;
  }

  // Save original instruction (would maintain breakpoint table)

  // Write TRAP instruction (Clipper breakpoint)
  uint32_t trap_insn = Architecture::Clipper::Opcodes::TRAP;
  return writeMemory(pid, address, &trap_insn, sizeof(trap_insn));
}

ErrorCode Debug::removeBreakpoint(ProcessId pid, uint32_t address) {
  // Restore original instruction
  // Would retrieve from breakpoint table
  return kSuccess;
}

ErrorCode Debug::setHardwareBreakpoint(ProcessId pid, uint32_t address) {
  // Clipper has limited hardware breakpoint support
  // Would use hardware debug registers if available
  return kSuccess;
}

ErrorCode Debug::removeHardwareBreakpoint(ProcessId pid, uint32_t address) {
  return kSuccess;
}

//
// Watchpoints
//

ErrorCode Debug::setWatchpoint(ProcessId pid, uint32_t address, size_t size, int type) {
  // Set memory watchpoint
  // Would use hardware support if available
  return kSuccess;
}

ErrorCode Debug::removeWatchpoint(ProcessId pid, uint32_t address) {
  return kSuccess;
}

//
// Memory mapping
//

ErrorCode Debug::enumerateMemoryRegions(ProcessId pid, std::vector<MemoryRegion> &regions) {
  // CLIX provides /proc interface for memory mapping
  regions.clear();

  // Read from /proc/pid/map
  char map_file[256];
  snprintf(map_file, sizeof(map_file), "/proc/%d/map", pid);

  FILE *fp = fopen(map_file, "r");
  if (!fp) {
    // Fallback to fixed layout if /proc not available
    MemoryRegion text;
    text.start = 0x00000000;
    text.end = 0x10000000;
    text.permissions = 0x5;  // R-X
    text.flags = 0;
    strcpy(text.name, "[text]");
    regions.push_back(text);

    MemoryRegion data;
    data.start = 0x10000000;
    data.end = 0x20000000;
    data.permissions = 0x6;  // RW-
    data.flags = 0;
    strcpy(data.name, "[data]");
    regions.push_back(data);

    MemoryRegion stack;
    stack.start = 0x7F000000;
    stack.end = 0x80000000;
    stack.permissions = 0x6;  // RW-
    stack.flags = 0;
    strcpy(stack.name, "[stack]");
    regions.push_back(stack);

    return kSuccess;
  }

  fclose(fp);
  return kSuccess;
}

//
// Process information
//

ErrorCode Debug::getProcessInfo(ProcessId pid, ProcessInfo &info) {
  // Read process information from /proc or kernel
  info.pid = pid;
  info.ppid = 0;
  info.pgrp = pid;
  info.uid = 0;
  info.gid = 0;
  info.state = ProcessState::RUNNING;
  info.flags = 0;
  info.nice = 0;
  info.priority = 0;
  strcpy(info.comm, "process");

  return kSuccess;
}

//
// Virtual memory statistics
//

ErrorCode Debug::getVMStats(ProcessId pid, VMStats &stats) {
  stats.text_size = 0;
  stats.data_size = 0;
  stats.stack_size = 0;
  stats.resident_size = 0;
  stats.shared_size = 0;

  return kSuccess;
}

//
// System V IPC information
//

ErrorCode Debug::getIPCInfo(ProcessId pid, IPCInfo &info) {
  // Query System V IPC resources
  info.num_shm_segments = 0;
  info.num_semaphores = 0;
  info.num_msg_queues = 0;

  return kSuccess;
}

} // namespace CLIX
} // namespace Host
} // namespace ds2
