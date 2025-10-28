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

#include <OS.h>
#include <debugger.h>

namespace ds2 {
namespace Host {
namespace BeOS {

class Platform : public ds2::Host::Platform {
public:
  // Process management
  static ErrorCode GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static ErrorCode GetThreadInfo(ProcessThreadId const &ptid, ThreadInfo &info);

  // Team (process) operations
  static team_id GetCurrentTeam();
  static ErrorCode InstallTeamDebugger(team_id team, port_id debuggerPort);
  static ErrorCode RemoveTeamDebugger(team_id team);

  // Debug event handling
  static ErrorCode WaitForDebugEvent(port_id debuggerPort, debug_debugger_message &event,
                                     bigtime_t timeout = B_INFINITE_TIMEOUT);
  static ErrorCode ContinueThread(thread_id thread);
  static ErrorCode StopThread(thread_id thread);

  // Thread state
  static ErrorCode GetThreadState(thread_id thread, debug_cpu_state &state);
  static ErrorCode SetThreadState(thread_id thread, const debug_cpu_state &state);

  // Memory operations
  static ErrorCode ReadMemory(team_id team, Address const &address, void *buffer,
                              size_t length, size_t *nread = nullptr);
  static ErrorCode WriteMemory(team_id team, Address const &address, void const *buffer,
                               size_t length, size_t *nwritten = nullptr);

  // Image (shared library) operations
  static ErrorCode GetLoadedImages(team_id team, std::vector<image_info> &images);
  static ErrorCode GetImageInfo(image_id image, image_info &info);

  // Breakpoints and watchpoints
  static ErrorCode SetBreakpoint(thread_id thread, Address const &address);
  static ErrorCode RemoveBreakpoint(thread_id thread, Address const &address);
  static ErrorCode SetWatchpoint(thread_id thread, Address const &address,
                                 size_t size, uint32_t mode);
  static ErrorCode RemoveWatchpoint(thread_id thread, Address const &address);

  // Signal/exception handling
  static ErrorCode HandleException(thread_id thread, int32 exception,
                                   debug_exception_type type);

  // Area (memory region) operations
  static ErrorCode GetAreas(team_id team, std::vector<area_info> &areas);

  // BeOS/Haiku specific helpers
  static const char *GetExceptionName(int32 exception);
  static bool IsHaiku(); // Distinguish Haiku from BeOS
};

} // namespace BeOS
} // namespace Host
} // namespace ds2
