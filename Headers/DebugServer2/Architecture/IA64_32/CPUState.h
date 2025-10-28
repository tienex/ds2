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

#include "DebugServer2/Architecture/IA64/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace IA64_32 {

//
// IA-64 32-bit mode (ILP32)
// Uses 32-bit pointers and integers but full IA-64 register set
// Supported on HP-UX and Windows for compatibility
//

// The CPU state is identical to IA64 at the hardware level
// The difference is in the ABI (32-bit pointers/ints)
using CPUState = IA64::CPUState;
using FPRegister = IA64::FPRegister;

} // namespace IA64_32
} // namespace Architecture
} // namespace ds2
