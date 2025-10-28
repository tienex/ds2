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

#include "DebugServer2/Base.h"

#if defined(OS_SYSV)

#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <sys/procfs.h>

namespace ds2 {
namespace Host {
namespace SystemV {

// Base class for System V procfs-based debugging
// Used by AIX, HP-UX, IRIX, Solaris, OpenServer, UnixWare
// These systems use /proc filesystem for debugging instead of ptrace()
class ProcFS {
public:
  virtual ~ProcFS() = default;

public:
  virtual ErrorCode wait(ProcessThreadId const &ptid, int *status = nullptr);

public:
  virtual ErrorCode traceMe(bool disableASLR);
  virtual ErrorCode traceThat(ProcessId pid) = 0;

public:
  virtual ErrorCode attach(ProcessId pid);
  virtual ErrorCode detach(ProcessId pid);

public:
  virtual ErrorCode kill(ProcessThreadId const &ptid, int signal) = 0;

public:
  virtual ErrorCode readString(ProcessThreadId const &ptid,
                               Address const &address, std::string &str,
                               size_t length, size_t *nread = nullptr) = 0;
  virtual ErrorCode readMemory(ProcessThreadId const &ptid,
                               Address const &address, void *buffer,
                               size_t length, size_t *nread = nullptr) = 0;
  virtual ErrorCode writeMemory(ProcessThreadId const &ptid,
                                Address const &address, void const *buffer,
                                size_t length, size_t *nwritten = nullptr) = 0;

public:
  virtual ErrorCode readCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &info,
                                 Architecture::CPUState &state) = 0;
  virtual ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                  ProcessInfo const &info,
                                  Architecture::CPUState const &state) = 0;

public:
  virtual ErrorCode suspend(ProcessThreadId const &ptid);

public:
  virtual ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal = 0, Address const &address = Address());
  virtual ErrorCode resume(ProcessThreadId const &ptid,
                           ProcessInfo const &pinfo, int signal = 0,
                           Address const &address = Address());

public:
  virtual ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) = 0;

public:
  virtual ErrorCode execute(ProcessThreadId const &ptid,
                            ProcessInfo const &pinfo, void const *code,
                            size_t length, uint64_t &result);

protected:
  // Helper functions for procfs operations
  static int openProc(ProcessId pid, const char *file, int flags);
  static ErrorCode controlProc(int ctlfd, long cmd, void *data = nullptr,
                              size_t dataSize = 0);
  static ErrorCode waitProc(int ctlfd);

protected:
  // Platform-specific implementations must provide these
  virtual long wrapProcfsControl(int ctlfd, long cmd, void *data) = 0;
};

} // namespace SystemV
} // namespace Host
} // namespace ds2

#endif // OS_SYSV
