//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Cell Broadband Engine definitions for Linux
//

#pragma once

#include <elf.h>

// Cell SPU note types for ptrace GETREGSET/SETREGSET
// These may not be defined in older kernel headers
#ifndef NT_SPU
#define NT_SPU 0x101  // SPU general registers
#endif

#ifndef NT_SPU_PC
#define NT_SPU_PC 0x102  // SPU program counter
#endif

#ifndef NT_SPU_MFC
#define NT_SPU_MFC 0x103  // SPU MFC (Memory Flow Controller) command queue
#endif

#ifndef NT_SPU_LSLR
#define NT_SPU_LSLR 0x104  // SPU Local Store Limit Register
#endif

#ifndef NT_SPU_DECR
#define NT_SPU_DECR 0x105  // SPU decrementer
#endif

#ifndef NT_SPU_EVENT_MASK
#define NT_SPU_EVENT_MASK 0x106  // SPU event mask
#endif

#ifndef NT_SPU_EVENT_STATUS
#define NT_SPU_EVENT_STATUS 0x107  // SPU event status
#endif

#ifndef NT_SPU_TAG_MASK
#define NT_SPU_TAG_MASK 0x108  // SPU tag mask
#endif

// Cell PPU note types (same as standard PowerPC but included for completeness)
#ifndef NT_PPC_VMX
#define NT_PPC_VMX 0x100  // PowerPC AltiVec/VMX registers
#endif

#ifndef NT_PPC_VSX
#define NT_PPC_VSX 0x102  // PowerPC VSX registers (POWER7+)
#endif

namespace ds2 {
namespace Host {
namespace Linux {
namespace Cell {

// SPU context creation flags
enum SPUContextFlags {
  SPU_CREATE_EVENTS_ENABLED = 0x0001,
  SPU_CREATE_GANG = 0x0002,
  SPU_CREATE_NOSCHED = 0x0004,
  SPU_CREATE_ISOLATE = 0x0008,
  SPU_CREATE_AFFINITY_SPU = 0x0010,
  SPU_CREATE_AFFINITY_MEM = 0x0020,
};

// SPU run control
enum SPURunControl {
  SPU_RUN_WAIT = 0x0001,
  SPU_RUN_NOTIFY_ACTIVE = 0x0002,
};

// SPU status codes
enum SPUStatus {
  SPU_STATUS_STOPPED = 0x00,
  SPU_STATUS_RUNNING = 0x01,
  SPU_STATUS_STOPPED_BY_STOP = 0x02,
  SPU_STATUS_STOPPED_BY_HALT = 0x04,
  SPU_STATUS_WAITING_FOR_CHANNEL = 0x08,
  SPU_STATUS_SINGLE_STEP = 0x10,
  SPU_STATUS_INVALID_INSTRUCTION = 0x20,
  SPU_STATUS_ISOLATED_CONTEXT = 0x40,
};

} // namespace Cell
} // namespace Linux
} // namespace Host
} // namespace ds2
