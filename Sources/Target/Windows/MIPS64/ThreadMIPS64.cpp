//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Host/Platform.h"

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Windows {

// NOTE: Windows never officially supported MIPS64
// This is a stub implementation that returns kErrorUnsupported

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // Windows does not support MIPS64
  return kErrorUnsupported;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  // Windows does not support MIPS64
  return kErrorUnsupported;
}

} // namespace Windows
} // namespace Target
} // namespace ds2
