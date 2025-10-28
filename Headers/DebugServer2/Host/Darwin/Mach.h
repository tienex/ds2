//
// Copyright (c) 2015 Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Darwin Mach - uses unified Mach implementation

#pragma once

#include "DebugServer2/Host/Mach/Mach.h"

namespace ds2 {
namespace Host {
namespace Darwin {

// Use the unified Mach interface
using Mach = ::ds2::Host::Mach::MachInterface;

} // namespace Darwin
} // namespace Host
} // namespace ds2
