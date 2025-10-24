//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Solaris/Process.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Target::Solaris::Process;

namespace ds2 {
namespace Target {
namespace Solaris {

// Solaris X86_64 (64-bit) process operations
// Solaris uses procfs for debugging

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  DS2LOG(Debug, "Solaris X86_64 allocateMemory via procfs");
  return kErrorUnsupported;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  DS2LOG(Debug, "Solaris X86_64 deallocateMemory via procfs");
  return kErrorUnsupported;
}

} // namespace Solaris
} // namespace Target
} // namespace ds2
