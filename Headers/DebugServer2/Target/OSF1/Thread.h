//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 Thread - Mach microkernel based
//

#pragma once

#include "DebugServer2/Target/OSF1/Process.h"
#include "DebugServer2/Target/POSIX/Thread.h"

namespace ds2 {
namespace Target {
namespace OSF1 {

class Process;

class Thread : public POSIX::Thread {
public:
  Thread(Process *process, ThreadId tid);
  virtual ~Thread();

protected:
  ErrorCode updateStopInfo(int waitStatus) override;

public:
  ErrorCode terminate() override;
  ErrorCode suspend() override;

public:
  ErrorCode readCPUState(Architecture::CPUState &state) override;
  ErrorCode writeCPUState(Architecture::CPUState const &state) override;

public:
  ErrorCode step(int signal = 0, Address const &address = Address()) override;
  ErrorCode resume(int signal = 0, Address const &address = Address()) override;

public:
  ErrorCode afterResume();
};

} // namespace OSF1
} // namespace Target
} // namespace ds2
