//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// GNU/Hurd Mach interface - similar to Darwin's but for GNU Mach
//

#pragma once

#include "DebugServer2/Architecture/CPUState.h"

// GNU Mach headers
#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Hurd {

//
// GNU/Hurd Mach debugger interface
// Based on Darwin's Mach interface but adapted for GNU Mach
//
class Mach {
public:
  virtual ~Mach() = default;

public:
  ErrorCode readMemory(ProcessThreadId const &ptid, Address const &address,
                       void *buffer, size_t length, size_t *nread = nullptr);
  ErrorCode writeMemory(ProcessThreadId const &ptid, Address const &address,
                        void const *buffer, size_t length,
                        size_t *nwritten = nullptr);

public:
  ErrorCode readCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                         Architecture::CPUState &state);
  ErrorCode writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                          Architecture::CPUState const &state);

public:
  ErrorCode suspend(ProcessThreadId const &ptid);

public:
  ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                 int signal = 0, Address const &address = Address());
  ErrorCode resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                   int signal = 0, Address const &address = Address());

public:
  ErrorCode getProcessMemoryRegion(ProcessId pid, Address const &address,
                                   MemoryRegionInfo &info);

public:
  ErrorCode getThreadInfo(ProcessThreadId const &tid, void *info);

private:
  task_t getMachTask(ProcessId pid);
  thread_t getMachThread(ProcessThreadId const &tid);
};

} // namespace Hurd
} // namespace Host
} // namespace ds2
