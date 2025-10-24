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
namespace ELKS {

// ELKS (Embeddable Linux Kernel Subset) Debugging Support
// 16-bit Unix-like OS for 8086/80286
// Uses a.out16 executable format

// Process and thread IDs
typedef int16_t pid_t;
typedef int16_t tid_t;

// Debug trace request codes
enum TraceRequest {
  PTRACE_TRACEME = 0,
  PTRACE_PEEKTEXT = 1,
  PTRACE_PEEKDATA = 2,
  PTRACE_PEEKUSER = 3,
  PTRACE_POKETEXT = 4,
  PTRACE_POKEDATA = 5,
  PTRACE_POKEUSER = 6,
  PTRACE_CONT = 7,
  PTRACE_KILL = 8,
  PTRACE_SINGLESTEP = 9,
};

// Signal numbers
enum Signals {
  SIGHUP = 1,
  SIGINT = 2,
  SIGQUIT = 3,
  SIGILL = 4,
  SIGTRAP = 5,
  SIGABRT = 6,
  SIGBUS = 7,
  SIGFPE = 8,
  SIGKILL = 9,
  SIGUSR1 = 10,
  SIGSEGV = 11,
  SIGUSR2 = 12,
  SIGPIPE = 13,
  SIGALRM = 14,
  SIGTERM = 15,
};

// a.out16 executable format (16-bit a.out variant)
struct aout16_header {
  uint8_t a_magic[2];     // Magic number (0x01, 0x03 for OMAGIC)
  uint8_t a_flags;        // Flags
  uint8_t a_cpu;          // CPU type (0=8086, 1=80186, 2=80286)
  uint16_t a_hdrlen;      // Header length
  uint8_t a_unused;       // Unused
  uint8_t a_version;      // Version
  uint32_t a_text;        // Text segment size
  uint32_t a_data;        // Data segment size
  uint32_t a_bss;         // BSS segment size
  uint32_t a_entry;       // Entry point (segment:offset)
  uint16_t a_total;       // Total memory required
  uint32_t a_syms;        // Symbol table size
};

// a.out16 magic numbers
#define AOUT16_OMAGIC  0x0107   // Old magic - text and data not separated
#define AOUT16_NMAGIC  0x0108   // New magic - read-only text
#define AOUT16_ZMAGIC  0x010B   // Demand paging

// Memory segments in ELKS
struct MemorySegment {
  uint16_t segment;       // Segment selector
  uint16_t offset;        // Offset within segment
  uint16_t size;          // Segment size
  uint8_t flags;          // Access flags
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Process control
  static ErrorCode attach(pid_t pid);
  static ErrorCode detach(pid_t pid);
  static ErrorCode wait(pid_t pid, int *status);
  static ErrorCode kill(pid_t pid, int signal);

  // Execution control
  static ErrorCode cont(pid_t pid, int signal = 0);
  static ErrorCode singleStep(pid_t pid, int signal = 0);

  // Breakpoint support
  static ErrorCode setBreakpoint(pid_t pid, Address address);
  static ErrorCode clearBreakpoint(pid_t pid, Address address);

  // Memory operations (16-bit segmented addressing)
  static ErrorCode readMemory(pid_t pid, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(pid_t pid, Address address, void const *data,
                              size_t size);

  // Register operations
  static ErrorCode getRegisters(pid_t pid, void *registers, size_t size);
  static ErrorCode setRegisters(pid_t pid, void const *registers, size_t size);

  // Process information
  static ErrorCode getProcessInfo(pid_t pid, ProcessInfo &info);

  // a.out16 format support
  static ErrorCode readAout16Header(pid_t pid, aout16_header &header);
  static ErrorCode getSegmentInfo(pid_t pid, MemorySegment &text,
                                 MemorySegment &data, MemorySegment &stack);

private:
  static bool _initialized;
};

} // namespace ELKS
} // namespace Host
} // namespace ds2
