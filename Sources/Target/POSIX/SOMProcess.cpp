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
//

#include "DebugServer2/Target/POSIX/SOMProcess.h"
#include "DebugServer2/Utils/Log.h"

#include <limits>

#define super ds2::Target::POSIX::Process

namespace ds2 {
namespace Target {
namespace POSIX {

//
// SOM Process Implementation
//
// SOM (System Object Module) is HP's binary format for PA-RISC.
// It was used on HP-UX before the migration to Itanium/ELF.
//
// Key SOM features:
// - PA-RISC specific relocations and calling conventions
// - Space-based addressing model
// - Shared libraries (.sl files)
// - Import/export tables
//

ErrorCode SOMProcess::updateInfo() {
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  return kSuccess;
}

ErrorCode SOMProcess::getSharedLibraryInfoAddress(Address &address) {
  // SOM shared library support
  // HP-UX uses shl_* APIs for shared library management

  if (_sharedLibraryInfoAddress.valid()) {
    address = _sharedLibraryInfoAddress;
    return kSuccess;
  }

  DS2LOG(Debug, "SOM shared library enumeration not yet implemented");
  return kErrorUnsupported;
}

ErrorCode SOMProcess::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {

  // SOM shared library enumeration
  // HP-UX provides shl_get() and shl_gethandle() to enumerate loaded libraries
  // Full implementation would:
  // 1. Use shl_get() to iterate through loaded modules
  // 2. Extract library names and addresses
  // 3. Call the callback for each library

  DS2LOG(Debug, "SOM shared library enumeration not yet implemented");
  return kSuccess; // Return success with empty list for now
}

} // namespace POSIX
} // namespace Target
} // namespace ds2
