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

// OpenServer supports both COFF and ELF
#include "DebugServer2/Target/POSIX/ELFProcess.h"

namespace ds2 {
namespace Target {
namespace OpenServer {

class Process : public POSIX::ELFProcess {
public:
  Process();
  ~Process() override;

protected:
  ErrorCode updateInfo() override;
};

} // namespace OpenServer
} // namespace Target
} // namespace ds2
