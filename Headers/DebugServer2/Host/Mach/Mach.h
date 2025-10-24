//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Unified Mach interface for Darwin (XNU Mach) and GNU/Hurd (GNU Mach)
//

#pragma once

#include "DebugServer2/Architecture/CPUState.h"

// Platform-specific Mach headers
#if defined(__APPLE__)
// Darwin (XNU Mach)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#elif defined(__GNU__)
// GNU/Hurd (GNU Mach)
#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#endif

#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Mach {

//
// Unified Mach debugger interface
// Works with both Darwin (XNU Mach) and GNU/Hurd (GNU Mach)
//
class MachInterface {
public:
  virtual ~MachInterface() = default;

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
#if defined(__APPLE__)
  // Darwin-specific APIs
  ErrorCode getProcessDylbInfo(ProcessId pid, Address &address);
  ErrorCode getThreadInfo(ProcessThreadId const &tid, thread_basic_info_t info);
  ErrorCode getThreadIdentifierInfo(ProcessThreadId const &tid,
                                    thread_identifier_info_data_t *threadID);
#elif defined(__GNU__)
  // GNU/Hurd-specific APIs
  ErrorCode getThreadInfo(ProcessThreadId const &tid, void *info);
#endif

protected:
  task_t getMachTask(ProcessId pid);
  thread_t getMachThread(ProcessThreadId const &tid);
};

} // namespace Mach
} // namespace Host
} // namespace ds2
