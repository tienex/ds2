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

#if defined(OS_AIX)

#include "DebugServer2/Host/SystemV/ProcFS.h"

namespace ds2 {
namespace Host {
namespace AIX {

//
// AIX-specific ProcFS implementation
// AIX uses /proc filesystem with its own specific control interface
//
class ProcFS : public SystemV::ProcFS {
public:
  ErrorCode traceThat(ProcessId pid) override;
  ErrorCode kill(ProcessThreadId const &ptid, int signal) override;

  ErrorCode readString(ProcessThreadId const &ptid, Address const &address,
                      std::string &str, size_t length,
                      size_t *nread = nullptr) override;
  ErrorCode readMemory(ProcessThreadId const &ptid, Address const &address,
                      void *buffer, size_t length,
                      size_t *nread = nullptr) override;
  ErrorCode writeMemory(ProcessThreadId const &ptid, Address const &address,
                       void const *buffer, size_t length,
                       size_t *nwritten = nullptr) override;

  ErrorCode readCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                        Architecture::CPUState &state) override;
  ErrorCode writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                         Architecture::CPUState const &state) override;

  ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) override;

protected:
  long wrapProcfsControl(int ctlfd, long cmd, void *data) override;
};

} // namespace AIX
} // namespace Host
} // namespace ds2

#endif // OS_AIX
