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
namespace XENIX {

// Microsoft XENIX Unix System V Debugging Support
// Supports x.out executable format
// Platforms: x86, x86-16 (286), m68k, z8000, VAX

// Process and thread IDs
typedef int32_t pid_t;
typedef int32_t tid_t;

// Debug trace request codes (for ptrace)
enum TraceRequest {
  PTRACE_TRACEME = 0,        // Allow parent to trace this process
  PTRACE_PEEKTEXT = 1,       // Read word from text segment
  PTRACE_PEEKDATA = 2,       // Read word from data segment
  PTRACE_PEEKUSER = 3,       // Read word from user area
  PTRACE_POKETEXT = 4,       // Write word to text segment
  PTRACE_POKEDATA = 5,       // Write word to data segment
  PTRACE_POKEUSER = 6,       // Write word to user area
  PTRACE_CONT = 7,           // Continue execution
  PTRACE_KILL = 8,           // Terminate traced process
  PTRACE_SINGLESTEP = 9,     // Single step execution
  PTRACE_ATTACH = 10,        // Attach to running process
  PTRACE_DETACH = 11,        // Detach from process
  PTRACE_GETREGS = 12,       // Get general registers
  PTRACE_SETREGS = 13,       // Set general registers
  PTRACE_GETFPREGS = 14,     // Get FP registers
  PTRACE_SETFPREGS = 15,     // Set FP registers
};

// Signal numbers (XENIX/Unix signals)
enum Signals {
  SIGHUP = 1,      // Hangup
  SIGINT = 2,      // Interrupt
  SIGQUIT = 3,     // Quit
  SIGILL = 4,      // Illegal instruction
  SIGTRAP = 5,     // Trace/breakpoint trap
  SIGABRT = 6,     // Abort
  SIGEMT = 7,      // Emulator trap
  SIGFPE = 8,      // Floating point exception
  SIGKILL = 9,     // Kill (cannot be caught)
  SIGBUS = 10,     // Bus error
  SIGSEGV = 11,    // Segmentation fault
  SIGSYS = 12,     // Bad system call
  SIGPIPE = 13,    // Broken pipe
  SIGALRM = 14,    // Alarm clock
  SIGTERM = 15,    // Termination
};

// Wait status macros
#define WIFEXITED(status)    (((status) & 0xFF) == 0)
#define WEXITSTATUS(status)  (((status) >> 8) & 0xFF)
#define WIFSIGNALED(status)  (((status) & 0xFF) != 0 && ((status) & 0x7F) != 0)
#define WTERMSIG(status)     ((status) & 0x7F)
#define WIFSTOPPED(status)   (((status) & 0xFF) == 0x7F)
#define WSTOPSIG(status)     (((status) >> 8) & 0xFF)

// x.out executable format structures
struct xout_header {
  uint16_t x_magic;       // Magic number (0x0206 for x.out)
  uint16_t x_ext;         // Extension type
  uint32_t x_text;        // Text segment size
  uint32_t x_data;        // Data segment size
  uint32_t x_bss;         // BSS segment size
  uint32_t x_syms;        // Symbol table size
  uint32_t x_reloc;       // Relocation table size
  uint32_t x_entry;       // Entry point address
  uint16_t x_cpu;         // CPU type
  uint16_t x_relsym;      // Relocation symbol offset
  uint32_t x_renv;        // Runtime environment
};

// x.out magic numbers
#define XOUT_MAGIC     0x0206   // Standard x.out
#define XOUT_MAGIC_286 0x0207   // 80286 protected mode

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

  // Memory operations
  static ErrorCode readMemory(pid_t pid, Address address, void *data,
                             size_t size);
  static ErrorCode writeMemory(pid_t pid, Address address, void const *data,
                              size_t size);

  // Register operations (architecture-specific)
  static ErrorCode getRegisters(pid_t pid, void *registers, size_t size);
  static ErrorCode setRegisters(pid_t pid, void const *registers, size_t size);

  // Process information
  static ErrorCode getProcessInfo(pid_t pid, ProcessInfo &info);

  // x.out format support
  static ErrorCode readXoutHeader(pid_t pid, xout_header &header);
  static ErrorCode getTextBase(pid_t pid, Address &address);
  static ErrorCode getDataBase(pid_t pid, Address &address);
  static ErrorCode getStackBase(pid_t pid, Address &address);

private:
  static bool _initialized;
};

} // namespace XENIX
} // namespace Host
} // namespace ds2
