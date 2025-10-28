//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/IRIX/Thread.h"
#include "DebugServer2/Target/IRIX/Process.h"

#define super ds2::Target::POSIX::Thread

namespace ds2 {
namespace Target {
namespace IRIX {

Thread::Thread(Process *process, ThreadId tid) : super(process, tid) {}

Thread::~Thread() {}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  return super::updateStopInfo(waitStatus);
}

ErrorCode Thread::step(int signal, Address const &address) {
  // IRIX uses procfs - implementation would go through /proc/<pid>/ctl
  return super::step(signal, address);
}

ErrorCode Thread::resume(int signal, Address const &address) {
  // IRIX uses procfs - implementation would go through /proc/<pid>/ctl
  return super::resume(signal, address);
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // IRIX procfs provides CPU state through /proc/<pid>/status
  return super::readCPUState(state);
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  // IRIX procfs allows CPU state writes through /proc/<pid>/ctl
  return super::writeCPUState(state);
}

} // namespace IRIX
} // namespace Target
} // namespace ds2
