//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// SOM (System Object Module) Process Support
// Used by HP-UX on PA-RISC
//

#pragma once

#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace POSIX {

//
// SOM Process
//
// SOM (System Object Module) is the binary format used by:
// - HP-UX on PA-RISC (32-bit and 64-bit)
//
// SOM is HP's proprietary format for PA-RISC architecture.
// Later HP-UX versions on Itanium use ELF instead.
//
class SOMProcess : public POSIX::Process {
protected:
  Address _sharedLibraryInfoAddress;

public:
  virtual ErrorCode getSharedLibraryInfoAddress(Address &address);
  ErrorCode enumerateSharedLibraries(
      std::function<void(SharedLibraryInfo const &)> const &cb) override;

protected:
  ErrorCode updateInfo() override;
};

} // namespace POSIX
} // namespace Target
} // namespace ds2
