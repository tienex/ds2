//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Alpha Software Breakpoint Manager
//

#include "DebugServer2/Core/SoftwareBreakpointManager.h"

namespace ds2 {

// Alpha breakpoint instruction (BPT - 0x00000080)
static const uint8_t gBreakpointCode[] = {0x80, 0x00, 0x00, 0x00};

size_t SoftwareBreakpointManager::getBreakpointSize() const {
  return sizeof(gBreakpointCode);
}

void SoftwareBreakpointManager::enableBreakpoint(BreakpointSite *site) {
  process()->writeMemory(site->address(), gBreakpointCode,
                         sizeof(gBreakpointCode), nullptr);
}

bool SoftwareBreakpointManager::hit(Target::Thread *thread) {
  // On Alpha, the PC points to the instruction after the breakpoint
  // when a breakpoint is hit, so we need to adjust it back
  Architecture::CPUState state;
  if (thread->readCPUState(state) != kSuccess)
    return false;

  return process()->breakpointHit(state.pc());
}

void SoftwareBreakpointManager::fixupBreakpoint(Target::Thread *thread) {
  Architecture::CPUState state;
  if (thread->readCPUState(state) != kSuccess)
    return;

  // PC already points to the breakpoint on Alpha, no adjustment needed
}

} // namespace ds2
