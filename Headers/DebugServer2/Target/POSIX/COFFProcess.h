//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// COFF (Common Object File Format) Process Support
// Used by SCO OpenServer and UnixWare
//

#pragma once

#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace POSIX {

//
// COFF Process
//
// COFF (Common Object File Format) was used by:
// - SCO OpenServer
// - SCO UnixWare (also supports ELF)
// - Early AT&T System V Unix
//
// This is the original COFF format, not the extended variants like
// XCOFF (AIX) or ECOFF (MIPS/OSF1).
//
class COFFProcess : public POSIX::Process {
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
