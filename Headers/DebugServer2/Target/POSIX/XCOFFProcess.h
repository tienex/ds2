//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// XCOFF (Extended Common Object File Format) Process Support
// Used by IBM AIX
//

#pragma once

#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace POSIX {

//
// XCOFF Process
//
// XCOFF (Extended Common Object File Format) is the binary format used by:
// - IBM AIX (XCOFF32 and XCOFF64)
//
// XCOFF is based on COFF but with IBM-specific extensions for AIX features
// like dynamic linking, shared libraries, and loadable modules.
//
class XCOFFProcess : public POSIX::Process {
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
