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

#include "DebugServer2/Target/POSIX/Thread.h"

namespace ds2 {
namespace Target {
namespace Hurd {

class Thread : public ds2::Target::POSIX::Thread {
protected:
  friend class Process;
  int _lastSyscallNumber;

public:
  Thread(Process *process, ThreadId tid);

protected:
  virtual ErrorCode updateStopInfo(int waitStatus) override;

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state) override;
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state) override;

public:
  virtual void updateState() override;

protected:
  virtual void updateState(bool force);
};

} // namespace Hurd
} // namespace Target
} // namespace ds2
