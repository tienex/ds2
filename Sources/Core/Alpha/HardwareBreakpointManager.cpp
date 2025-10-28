//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Alpha Hardware Breakpoint Manager
//

#include "DebugServer2/Core/HardwareBreakpointManager.h"

namespace ds2 {

// Alpha doesn't have hardware breakpoint registers in the same way
// as x86 or ARM. Hardware breakpoints would need to be implemented
// via PALcode or other architecture-specific mechanisms.
// For now, we return unsupported.

size_t HardwareBreakpointManager::maxWatchpoints() { return 0; }

size_t HardwareBreakpointManager::maxBreakpoints() { return 0; }

ErrorCode HardwareBreakpointManager::add(Address const &address, Type type,
                                         size_t size, Mode mode) {
  return kErrorUnsupported;
}

ErrorCode HardwareBreakpointManager::remove(Address const &address) {
  return kErrorUnsupported;
}

bool HardwareBreakpointManager::hit(Target::Thread *thread) { return false; }

void HardwareBreakpointManager::enable(Target::Thread *thread) {}

void HardwareBreakpointManager::disable(Target::Thread *thread) {}

ErrorCode HardwareBreakpointManager::enableLocation(Site const &site,
                                                     Target::Thread *thread) {
  return kErrorUnsupported;
}

ErrorCode HardwareBreakpointManager::disableLocation(Site const &site,
                                                      Target::Thread *thread) {
  return kErrorUnsupported;
}

} // namespace ds2
