//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/OpenBSD/Thread.h"
#include "DebugServer2/Target/OpenBSD/Process.h"
#include "DebugServer2/Host/OpenBSD/PTrace.h"

#define super ds2::Target::POSIX::Thread

using ds2::Host::OpenBSD::PTrace;

namespace ds2 {
namespace Target {
namespace OpenBSD {

Thread::Thread(Process *process, ThreadId tid) : super(process, tid) {}

Thread::~Thread() {}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  return super::updateStopInfo(waitStatus);
}

ErrorCode Thread::step(int signal, Address const &address) {
  PTrace pt;
  return pt.step(ProcessThreadId(process()->pid(), tid()), process()->info(),
                 signal, address);
}

ErrorCode Thread::resume(int signal, Address const &address) {
  PTrace pt;
  return pt.resume(ProcessThreadId(process()->pid(), tid()),
                   process()->info(), signal, address);
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  PTrace pt;
  return pt.readCPUState(ProcessThreadId(process()->pid(), tid()),
                         process()->info(), state);
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  PTrace pt;
  return pt.writeCPUState(ProcessThreadId(process()->pid(), tid()),
                          process()->info(), state);
}

} // namespace OpenBSD
} // namespace Target
} // namespace ds2
