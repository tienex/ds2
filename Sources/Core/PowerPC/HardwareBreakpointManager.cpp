//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC Hardware Breakpoint Manager
//

#include "DebugServer2/Core/HardwareBreakpointManager.h"

namespace ds2 {

// PowerPC has hardware debugging features (DABR, IABR registers on some models),
// but implementation varies significantly across PowerPC variants:
// - PowerPC 6xx/7xx: IABR (Instruction Address Breakpoint Register)
// - PowerPC G4/G5: DABR (Data Address Breakpoint Register)
// - POWER4+: Multiple DABRs, enhanced debug features
// - POWER7+: DAWR (Data Address Watchpoint Register)
// - Book E (embedded): Different debug register set entirely
//
// For now, we return unsupported until platform-specific
// implementations are needed.

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
