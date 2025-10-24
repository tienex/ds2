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

#include "DebugServer2/Target/POSIX/XCOFFProcess.h"

namespace ds2 {
namespace Target {
namespace AIX {

class Process : public POSIX::XCOFFProcess {
public:
  Process();
  ~Process() override;

protected:
  ErrorCode updateInfo() override;
};

} // namespace AIX
} // namespace Target
} // namespace ds2
