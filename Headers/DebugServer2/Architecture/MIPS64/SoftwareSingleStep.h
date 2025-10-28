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
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Target/Process.h"

namespace ds2 {
namespace Architecture {
namespace MIPS64 {

ErrorCode PrepareMIPS64SoftwareSingleStep(Target::Process *process, uint64_t pc,
                                          Architecture::CPUState const &state,
                                          uint64_t &nextPC,
                                          uint32_t &nextPCSize,
                                          uint64_t &branchPC,
                                          uint32_t &branchPCSize);

ErrorCode PrepareMIPS16SoftwareSingleStep(Target::Process *process, uint64_t pc,
                                          Architecture::CPUState const &state,
                                          uint64_t &nextPC,
                                          uint32_t &nextPCSize,
                                          uint64_t &branchPC,
                                          uint32_t &branchPCSize);

ErrorCode PrepareMicroMIPSSoftwareSingleStep(
    Target::Process *process, uint64_t pc, Architecture::CPUState const &state,
    uint64_t &nextPC, uint32_t &nextPCSize, uint64_t &branchPC,
    uint32_t &branchPCSize);

ErrorCode PrepareSoftwareSingleStep(Target::Process *process,
                                    BreakpointManager *manager,
                                    CPUState const &state,
                                    Address const &address);
} // namespace MIPS64
} // namespace Architecture
} // namespace ds2
