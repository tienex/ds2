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
//

#include "DebugServer2/Target/POSIX/COFFProcess.h"
#include "DebugServer2/Utils/Log.h"

#include <limits>

#define super ds2::Target::POSIX::Process

namespace ds2 {
namespace Target {
namespace POSIX {

//
// COFF Process Implementation
//
// COFF (Common Object File Format) is the original AT&T binary format.
// It was used on early System V Unix and SCO systems.
//
// Key COFF features:
// - Simple header structure
// - Section table for code/data/bss
// - Symbol table and string table
// - Limited relocation support
//

ErrorCode COFFProcess::updateInfo() {
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  return kSuccess;
}

ErrorCode COFFProcess::getSharedLibraryInfoAddress(Address &address) {
  // COFF shared library support
  // Note: COFF has limited dynamic linking support
  // Most SCO systems transitioned to ELF for better shared library support

  if (_sharedLibraryInfoAddress.valid()) {
    address = _sharedLibraryInfoAddress;
    return kSuccess;
  }

  DS2LOG(Debug, "COFF shared library enumeration not yet implemented");
  return kErrorUnsupported;
}

ErrorCode COFFProcess::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {

  // COFF shared library enumeration
  // Early COFF had limited shared library support
  // Later SCO systems used ELF for better dynamic linking

  DS2LOG(Debug, "COFF shared library enumeration not yet implemented");
  return kSuccess; // Return success with empty list for now
}

} // namespace POSIX
} // namespace Target
} // namespace ds2
