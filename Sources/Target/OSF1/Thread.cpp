//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 Thread - adapted from Hurd's Mach-based implementation
//

#include "DebugServer2/Target/OSF1/Thread.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/syscall.h>
#include <sys/wait.h>

#define super ds2::Target::POSIX::Thread

namespace ds2 {
namespace Target {
namespace OSF1 {

Thread::Thread(Process *process, ThreadId tid)
    : super(process, tid) {}

Thread::~Thread() {}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  ProcessInfo info;
  ErrorCode error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  OSF1::Process *osf1Process = static_cast<OSF1::Process *>(_process);
  return osf1Process->mach().readCPUState(
      ProcessThreadId(osf1Process->pid(), tid()), info, state);
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  ProcessInfo info;
  ErrorCode error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  OSF1::Process *osf1Process = static_cast<OSF1::Process *>(_process);
  return osf1Process->mach().writeCPUState(
      ProcessThreadId(osf1Process->pid(), tid()), info, state);
}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  super::updateStopInfo(waitStatus);

  switch (_stopInfo.event) {
  case StopInfo::kEventNone:
    DS2LOG(Debug, "thread stopped for unknown reason, status=%#x", waitStatus);
    // fallthrough

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(_stopInfo.reason == StopInfo::kReasonNone);
    return kSuccess;

  case StopInfo::kEventStop: {
    // OSF/1: For now, assume breakpoint
    _stopInfo.reason = StopInfo::kReasonBreakpoint;
  } break;
  }

  return kSuccess;
}

ErrorCode Thread::terminate() {
  return super::terminate();
}

ErrorCode Thread::suspend() {
  OSF1::Process *osf1Process = static_cast<OSF1::Process *>(_process);
  return osf1Process->mach().suspend(
      ProcessThreadId(osf1Process->pid(), tid()));
}

ErrorCode Thread::step(int signal, Address const &address) {
  ProcessInfo info;
  OSF1::Process *osf1Process = static_cast<OSF1::Process *>(_process);

  CHK(osf1Process->getInfo(info));
  return osf1Process->mach().step(
      ProcessThreadId(osf1Process->pid(), tid()), info, signal, address);
}

ErrorCode Thread::resume(int signal, Address const &address) {
  ProcessInfo info;
  OSF1::Process *osf1Process = static_cast<OSF1::Process *>(_process);

  CHK(osf1Process->getInfo(info));
  return osf1Process->mach().resume(
      ProcessThreadId(osf1Process->pid(), tid()), info, signal, address);
}

ErrorCode Thread::afterResume() {
  return kSuccess;
}

} // namespace OSF1
} // namespace Target
} // namespace ds2
