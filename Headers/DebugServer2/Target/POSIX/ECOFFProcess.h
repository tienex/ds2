//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// ECOFF (Extended Common Object File Format) Process Support
// Used by OSF/1 (Digital UNIX / Tru64) and SGI IRIX
//

#pragma once

#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace POSIX {

//
// ECOFF Process
//
// ECOFF (Extended Common Object File Format) was the binary format used by:
// - OSF/1 / Digital UNIX / Tru64
// - SGI IRIX
// - Ultrix
// - Sony NEWS-OS
//
// It's simpler than ELF and was common on MIPS-based systems.
//
class ECOFFProcess : public POSIX::Process {
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
