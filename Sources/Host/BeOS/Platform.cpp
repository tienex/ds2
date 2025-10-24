//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/BeOS/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <errno.h>
#include <image.h>
#include <string.h>

namespace ds2 {
namespace Host {
namespace BeOS {

team_id Platform::GetCurrentTeam() { return find_thread(NULL); }

ErrorCode Platform::InstallTeamDebugger(team_id team, port_id debuggerPort) {
  if (install_team_debugger(team, debuggerPort) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::RemoveTeamDebugger(team_id team) {
  if (remove_team_debugger(team) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::WaitForDebugEvent(port_id debuggerPort,
                                      debug_debugger_message &event,
                                      bigtime_t timeout) {
  int32 code;
  ssize_t size = read_port_etc(debuggerPort, &code, &event, sizeof(event),
                               B_RELATIVE_TIMEOUT, timeout);

  if (size < 0) {
    if (size == B_TIMED_OUT || size == B_WOULD_BLOCK)
      return kErrorTimeout;
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode Platform::ContinueThread(thread_id thread) {
  if (resume_thread(thread) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::StopThread(thread_id thread) {
  if (suspend_thread(thread) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::GetThreadState(thread_id thread, debug_cpu_state &state) {
  if (get_thread_cpu_state(thread, &state) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::SetThreadState(thread_id thread,
                                   const debug_cpu_state &state) {
  if (set_thread_cpu_state(thread, &state) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::ReadMemory(team_id team, Address const &address,
                               void *buffer, size_t length, size_t *nread) {
  ssize_t result =
      debug_read_memory(team, (void *)address.value(), buffer, length);

  if (result < 0)
    return Platform::TranslateError();

  if (nread != nullptr)
    *nread = result;

  return kSuccess;
}

ErrorCode Platform::WriteMemory(team_id team, Address const &address,
                                void const *buffer, size_t length,
                                size_t *nwritten) {
  ssize_t result =
      debug_write_memory(team, (void *)address.value(), buffer, length);

  if (result < 0)
    return Platform::TranslateError();

  if (nwritten != nullptr)
    *nwritten = result;

  return kSuccess;
}

ErrorCode Platform::GetLoadedImages(team_id team,
                                    std::vector<image_info> &images) {
  image_info info;
  int32 cookie = 0;

  images.clear();

  while (get_next_image_info(team, &cookie, &info) == B_OK) {
    images.push_back(info);
  }

  return kSuccess;
}

ErrorCode Platform::GetImageInfo(image_id image, image_info &info) {
  if (get_image_info(image, &info) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::SetBreakpoint(thread_id thread, Address const &address) {
  if (set_debugger_breakpoint((void *)address.value()) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::RemoveBreakpoint(thread_id thread, Address const &address) {
  if (clear_debugger_breakpoint((void *)address.value()) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::SetWatchpoint(thread_id thread, Address const &address,
                                  size_t size, uint32_t mode) {
  // Haiku supports hardware watchpoints
  if (set_debugger_watchpoint((void *)address.value(), B_DATA_WRITE_WATCHPOINT,
                              size) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::RemoveWatchpoint(thread_id thread,
                                     Address const &address) {
  if (clear_debugger_watchpoint((void *)address.value()) != B_OK)
    return Platform::TranslateError();
  return kSuccess;
}

ErrorCode Platform::HandleException(thread_id thread, int32 exception,
                                    debug_exception_type type) {
  // Send continue signal to thread after handling exception
  debug_nub_continue_thread continueMsg;
  continueMsg.thread = thread;
  continueMsg.handle_event = B_THREAD_DEBUG_HANDLE_EVENT;

  // This would be sent via the debug port in actual implementation
  return kSuccess;
}

ErrorCode Platform::GetAreas(team_id team, std::vector<area_info> &areas) {
  area_info info;
  int32 cookie = 0;

  areas.clear();

  ssize_t result;
  while ((result = get_next_area_info(team, &cookie, &info)) == B_OK) {
    areas.push_back(info);
  }

  return kSuccess;
}

const char *Platform::GetExceptionName(int32 exception) {
  switch (exception) {
  case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
    return "THREAD_DEBUGGED";
  case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
    return "DEBUGGER_CALL";
  case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
    return "BREAKPOINT_HIT";
  case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
    return "WATCHPOINT_HIT";
  case B_DEBUGGER_MESSAGE_SINGLE_STEP:
    return "SINGLE_STEP";
  case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
    return "PRE_SYSCALL";
  case B_DEBUGGER_MESSAGE_POST_SYSCALL:
    return "POST_SYSCALL";
  case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
    return "SIGNAL_RECEIVED";
  case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
    return "EXCEPTION_OCCURRED";
  case B_DEBUGGER_MESSAGE_TEAM_CREATED:
    return "TEAM_CREATED";
  case B_DEBUGGER_MESSAGE_TEAM_DELETED:
    return "TEAM_DELETED";
  case B_DEBUGGER_MESSAGE_TEAM_EXEC:
    return "TEAM_EXEC";
  default:
    return "UNKNOWN";
  }
}

bool Platform::IsHaiku() {
#ifdef __HAIKU__
  return true;
#else
  return false;
#endif
}

ErrorCode Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  team_info teamInfo;
  if (get_team_info(pid, &teamInfo) != B_OK)
    return Platform::TranslateError();

  info.pid = teamInfo.team;
  info.nativePid = teamInfo.team;
  // BeOS/Haiku doesn't have a traditional parent/child relationship
  info.parentPid = 0;
  info.realUid = teamInfo.uid;
  info.realGid = teamInfo.gid;

  // Get team name
  strncpy(info.name, teamInfo.args, sizeof(info.name) - 1);
  info.name[sizeof(info.name) - 1] = '\0';

  return kSuccess;
}

ErrorCode Platform::GetThreadInfo(ProcessThreadId const &ptid,
                                  ThreadInfo &info) {
  thread_info threadInfo;
  if (get_thread_info(ptid.tid, &threadInfo) != B_OK)
    return Platform::TranslateError();

  info.tid = threadInfo.thread;
  strncpy(info.name, threadInfo.name, sizeof(info.name) - 1);
  info.name[sizeof(info.name) - 1] = '\0';

  // Map thread state
  switch (threadInfo.state) {
  case B_THREAD_RUNNING:
    info.state = kThreadStateRunning;
    break;
  case B_THREAD_READY:
    info.state = kThreadStateStopped;
    break;
  case B_THREAD_RECEIVING:
  case B_THREAD_ASLEEP:
  case B_THREAD_SUSPENDED:
  case B_THREAD_WAITING:
    info.state = kThreadStateSleeping;
    break;
  default:
    info.state = kThreadStateInvalid;
    break;
  }

  return kSuccess;
}

} // namespace BeOS
} // namespace Host
} // namespace ds2
