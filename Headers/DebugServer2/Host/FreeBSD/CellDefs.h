//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Cell Broadband Engine definitions for FreeBSD
//

#pragma once

#include <sys/ptrace.h>

// Cell-specific ptrace requests for FreeBSD
// These are custom extensions for Cell/PS3 support
#ifndef PT_GETVECREGS
#define PT_GETVECREGS 50  // Get AltiVec/VMX registers
#endif

#ifndef PT_SETVECREGS
#define PT_SETVECREGS 51  // Set AltiVec/VMX registers
#endif

#ifndef PT_GETSPUREGS
#define PT_GETSPUREGS 52  // Get SPU registers
#endif

#ifndef PT_SETSPUREGS
#define PT_SETSPUREGS 53  // Set SPU registers
#endif

#ifndef PT_GETSPUMFC
#define PT_GETSPUMFC 54  // Get SPU MFC command queue
#endif

#ifndef PT_SETSPUMFC
#define PT_SETSPUMFC 55  // Set SPU MFC command queue
#endif

namespace ds2 {
namespace Host {
namespace FreeBSD {
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
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
