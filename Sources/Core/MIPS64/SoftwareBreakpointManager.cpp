//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/Architecture/MIPS64/CPUState.h"
#include "DebugServer2/Architecture/MIPS/InstructionMode.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {

ErrorCode SoftwareBreakpointManager::add(Address const &address,
                                         Lifetime lifetime, size_t size,
                                         Mode mode) {
  if (size != 2 && size != 4) {
    // Determine instruction size based on mode bit in address
    // Bit 0 set means MIPS16 or microMIPS (16-bit or 32-bit)
    // Bit 0 clear means normal MIPS64 (32-bit instructions)

    if (address.value() & 1) {
      // MIPS16/microMIPS mode - typically 2 bytes for break
      size = 2;
    } else {
      // Normal MIPS64 - 4 bytes (instructions are still 32-bit)
      size = 4;
    }
  }

  // Clear mode bit before storing address
  return super::add(address.value() & ~1ULL, lifetime, size, mode);
}

ErrorCode SoftwareBreakpointManager::remove(Address const &address) {
  DS2ASSERT(!(address.value() & 1));
  return super::remove(address);
}

bool SoftwareBreakpointManager::has(Address const &address) const {
  DS2ASSERT(!(address.value() & 1));
  return super::has(address);
}

void SoftwareBreakpointManager::enumerate(
    std::function<void(Site const &)> const &cb) const {
  // Remove the mode bit if present
  super::enumerate([&](Site const &site) {
    if (site.address & 1) {
      Site copy(site);
      copy.address = copy.address.value() & ~1;
      cb(copy);
    } else {
      cb(site);
    }
  });
}

int SoftwareBreakpointManager::hit(Target::Thread *thread, Site &site) {
  ds2::Architecture::CPUState state;
  thread->readCPUState(state);

  // On MIPS64, when a breakpoint is hit, the PC points to the break instruction
  // (not past it like on x86). No adjustment needed on POSIX.
#if defined(OS_WIN32)
  // On Windows, we may need to adjust PC back
  // This depends on the Windows MIPS64 debugger behavior
  // For now, leave as-is and adjust if needed during testing
#endif

  return super::hit(state.pc(), site) ? 0 : -1;
}

void SoftwareBreakpointManager::getOpcode(uint32_t type,
                                          ByteVector &opcode) const {
  opcode.clear();

  // MIPS64 uses the same breakpoint instructions as MIPS32
  // Normal MIPS64 (4 bytes): break instruction with code
  //   Format: 000000 xxxxxxxxxx yyyyyyyyyy 001101
  //   Common values: 0x0005000d (break 5), 0x0001000d (break 1)
  //   We'll use 0x0005000d to match GDB
  //
  // MIPS16 (2 bytes): break16 instruction
  //   Format: 11100101xxxxxxxx
  //   We'll use 0xe805 (break 5)
  //
  // microMIPS (2 bytes): break16 instruction
  //   Format: 0100011000000101
  //   We'll use 0x4605

  switch (type) {
  case 2:
    // 2-byte breakpoint for MIPS16/microMIPS
    // MIPS16: 0xe805 (break 5)
    // Little-endian: 0x05, 0xe8
    // Big-endian: 0xe8, 0x05
    opcode.push_back('\xe8');
    opcode.push_back('\x05');
    break;

  case 4:
    // 4-byte breakpoint for normal MIPS64
    // 0x0005000d (break 5)
    // Little-endian: 0x0d, 0x00, 0x05, 0x00
    // Big-endian: 0x00, 0x05, 0x00, 0x0d
    opcode.push_back('\x00');
    opcode.push_back('\x05');
    opcode.push_back('\x00');
    opcode.push_back('\x0d');
    break;

  default:
    DS2LOG(Error, "invalid MIPS64 breakpoint type %d (expected 2 or 4)", type);
    DS2BUG("invalid breakpoint type");
    break;
  }

#if !(defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE))
#error "Target not supported."
#endif

#if defined(ENDIAN_LITTLE)
  // Reverse for little-endian
  std::reverse(opcode.begin(), opcode.end());
#endif
}

ErrorCode SoftwareBreakpointManager::isValid(Address const &address,
                                             size_t size, Mode mode) const {
  DS2ASSERT(mode == kModeExec);

  // MIPS64 supports 2-byte (MIPS16/microMIPS) and 4-byte (normal) breakpoints
  if (size != 2 && size != 4) {
    DS2LOG(Debug,
           "Received unsupported MIPS64 breakpoint size %zu (expected 2 or 4)",
           size);
    return kErrorInvalidArgument;
  }

  return super::isValid(address, size, mode);
}

size_t SoftwareBreakpointManager::chooseBreakpointSize() const {
  // Default to 4-byte MIPS64 breakpoint
  // The add() method will adjust based on mode bit if needed
  return 4;
}
} // namespace ds2
