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

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Types.h"

#include <string>
#include <vector>

namespace ds2 {
namespace Host {
namespace Plan9 {

// Plan 9 / Inferno debugging through /proc
// Plan 9 has a unique text-based /proc interface
// Files: /proc/<pid>/ctl, /proc/<pid>/text, /proc/<pid>/mem, /proc/<pid>/regs

class ProcFS {
public:
  // CPU state operations
  static ErrorCode readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state);

  static ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state);

  // Process control via /proc/<pid>/ctl
  // Control messages: "hang", "kill", "start", "waitstop", "wakeup", etc.
  static ErrorCode attach(ProcessId pid);
  static ErrorCode detach(ProcessId pid);

  static ErrorCode suspend(ProcessThreadId const &ptid);
  static ErrorCode resume(ProcessThreadId const &ptid);

  static ErrorCode step(ProcessThreadId const &ptid);
  static ErrorCode kill(ProcessId pid);

  // Breakpoints via text segment manipulation
  static ErrorCode setBreakpoint(ProcessThreadId const &ptid,
                                Address const &address);
  static ErrorCode removeBreakpoint(ProcessThreadId const &ptid,
                                   Address const &address);

  // Memory operations via /proc/<pid>/mem
  static ErrorCode readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *nread = nullptr);

  static ErrorCode writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *nwritten = nullptr);

  // Read text segment (code) via /proc/<pid>/text
  static ErrorCode readText(ProcessId pid, Address const &address,
                           void *buffer, size_t length,
                           size_t *nread = nullptr);

  // Process information via /proc/<pid>/status
  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);

  // Wait for process state change
  static ErrorCode wait(ProcessId pid, int *status);

  // Plan 9 specific: send control command
  static ErrorCode sendControlCommand(ProcessId pid, const char *command);

  // Plan 9 specific: read process status
  static ErrorCode readStatus(ProcessId pid, std::string &status);

  // Inferno specific support
  static bool isInferno();
  static ErrorCode readDisInfernoState(ProcessId pid, void *state,
                                      size_t stateSize);
};

} // namespace Plan9
} // namespace Host
} // namespace ds2
