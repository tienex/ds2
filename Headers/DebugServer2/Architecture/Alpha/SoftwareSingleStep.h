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

#include "DebugServer2/Architecture/CPUState.h"

namespace ds2 {

class BreakpointManager;

namespace Target {
class ProcessBase;
}

namespace Architecture {
namespace Alpha {

int PrepareSoftwareSingleStep(Target::ProcessBase *process,
                              BreakpointManager *manager,
                              CPUState const &state, Address const &address);

} // namespace Alpha
} // namespace Architecture
} // namespace ds2
