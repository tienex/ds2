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
namespace OpenVMS {

// OpenVMS debugging support
// Uses DEBUG and LIB$ system services
// Available on VAX, Alpha, IA-64 (Itanium), and x86-64

class Debug {
public:
  // Process control
  static ErrorCode attach(ProcessId pid);
  static ErrorCode detach(ProcessId pid);

  static ErrorCode suspend(ProcessThreadId const &ptid);
  static ErrorCode resume(ProcessThreadId const &ptid);

  static ErrorCode step(ProcessThreadId const &ptid);
  static ErrorCode kill(ProcessId pid);

  // CPU state operations
  static ErrorCode readCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState &state);

  static ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &pinfo,
                                 Architecture::CPUState const &state);

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

  // Process/thread information
  static ErrorCode getProcessInfo(ProcessId pid, ProcessInfo &info);
  static ErrorCode getThreadInfo(ProcessThreadId const &ptid, ThreadInfo &info);

  // OpenVMS specific: image (shared library) operations
  static ErrorCode getLoadedImages(ProcessId pid,
                                   std::vector<std::string> &images);

  // OpenVMS specific: condition handler
  static ErrorCode getConditionHandler(ProcessThreadId const &ptid,
                                       Address &handlerAddress);

  // OpenVMS specific: AST (Asynchronous System Trap) control
  static ErrorCode enableAST(ProcessThreadId const &ptid, bool enable);

  // Wait for debug event
  static ErrorCode wait(ProcessThreadId *ptid, int *status);

  // Signal/exception handling
  static ErrorCode sendSignal(ProcessThreadId const &ptid, int signal);
};

} // namespace OpenVMS
} // namespace Host
} // namespace ds2
