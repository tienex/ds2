//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 (Digital UNIX / Tru64) Mach interface
// OSF/1 uses Mach 2.5/3.0 microkernel
//

#pragma once

#include "DebugServer2/Host/Mach/Mach.h"

namespace ds2 {
namespace Host {
namespace OSF1 {

// Use the unified Mach interface
// OSF/1 uses Mach 2.5/3.0 microkernel with similar APIs to other Mach platforms
using Mach = ::ds2::Host::Mach::MachInterface;

} // namespace OSF1
} // namespace Host
} // namespace ds2
