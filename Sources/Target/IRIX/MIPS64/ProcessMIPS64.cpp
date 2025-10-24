//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/IRIX/Process.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Target::IRIX::Process;

namespace ds2 {
namespace Target {
namespace IRIX {

// IRIX MIPS64-specific process operations
// IRIX 6.5+ supports MIPS64 with procfs debugging

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  // IRIX procfs-based memory allocation for MIPS64
  DS2LOG(Debug, "IRIX MIPS64 allocateMemory not yet fully implemented");
  return kErrorUnsupported;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  // IRIX procfs-based memory deallocation for MIPS64
  DS2LOG(Debug, "IRIX MIPS64 deallocateMemory not yet fully implemented");
  return kErrorUnsupported;
}

} // namespace IRIX
} // namespace Target
} // namespace ds2
