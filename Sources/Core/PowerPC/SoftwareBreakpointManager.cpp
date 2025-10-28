//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC Software Breakpoint Manager
//

#include "DebugServer2/Core/SoftwareBreakpointManager.h"

namespace ds2 {

// PowerPC breakpoint instruction (trap word - tw 31, 0, 0)
// Encoding: 0x7FE00008 (opcode=31, TO=31, RA=0, RB=0, extended_opcode=4)
// This is a standard PowerPC trap instruction that debuggers recognize
// Note: Big-endian byte order
static const uint8_t gBreakpointCode[] = {0x7F, 0xE0, 0x00, 0x08};

size_t SoftwareBreakpointManager::getBreakpointSize() const {
  return sizeof(gBreakpointCode);
}

void SoftwareBreakpointManager::enableBreakpoint(BreakpointSite *site) {
  process()->writeMemory(site->address(), gBreakpointCode,
                         sizeof(gBreakpointCode), nullptr);
}

bool SoftwareBreakpointManager::hit(Target::Thread *thread) {
  // On PowerPC, when a breakpoint is hit, the PC points to the
  // breakpoint instruction itself (not the next instruction)
  Architecture::CPUState state;
  if (thread->readCPUState(state) != kSuccess)
    return false;

  return process()->breakpointHit(state.pc());
}

void SoftwareBreakpointManager::fixupBreakpoint(Target::Thread *thread) {
  Architecture::CPUState state;
  if (thread->readCPUState(state) != kSuccess)
    return;

  // PC points to the breakpoint instruction, no adjustment needed
  // The breakpoint will be replaced with the original instruction
  // and the thread will re-execute from the same PC
}

} // namespace ds2
