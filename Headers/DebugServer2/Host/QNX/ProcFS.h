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

#include <sys/debug.h>
#include <sys/procfs.h>
#include <vector>

namespace ds2 {
namespace Host {
namespace QNX {

// QNX Neutrino procfs-based debugging
// QNX 4 and QNX Neutrino (6+) use /proc filesystem for debugging
// Uses devctl() calls for thread control and register access

class ProcFS {
public:
  // CPU state operations
  static ErrorCode readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state);

  static ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state);

  // Process control
  static ErrorCode attach(ProcessId pid);
  static ErrorCode detach(ProcessId pid);

  static ErrorCode suspend(ProcessThreadId const &ptid);
  static ErrorCode resume(ProcessThreadId const &ptid, int signal = 0);

  static ErrorCode step(ProcessThreadId const &ptid, int signal = 0);

  // Breakpoints
  static ErrorCode setBreakpoint(ProcessThreadId const &ptid,
                                Address const &address);
  static ErrorCode removeBreakpoint(ProcessThreadId const &ptid,
                                   Address const &address);

  // Watchpoints
  static ErrorCode setWatchpoint(ProcessThreadId const &ptid,
                                Address const &address, size_t size,
                                uint32_t mode);
  static ErrorCode removeWatchpoint(ProcessThreadId const &ptid,
                                   Address const &address);

  // Memory operations
  static ErrorCode readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *nread = nullptr);

  static ErrorCode writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *nwritten = nullptr);

  // Thread enumeration
  static ErrorCode getThreadIds(ProcessId pid, std::vector<ThreadId> &tids);

  // Process/thread information
  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);
  static ErrorCode getThreadInfo(ProcessThreadId const &ptid, ThreadInfo &info);

  // Wait for events
  static ErrorCode wait(ProcessThreadId *ptid, int *status);

  // Signal handling
  static ErrorCode sendSignal(ProcessThreadId const &ptid, int signal);

  // QNX-specific: mapped objects (shared libraries)
  static ErrorCode getMappedObjects(ProcessId pid,
                                   std::vector<std::string> &objects);
};

} // namespace QNX
} // namespace Host
} // namespace ds2
