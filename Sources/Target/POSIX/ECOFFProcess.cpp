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
//

#include "DebugServer2/Target/POSIX/ECOFFProcess.h"
#include "DebugServer2/Utils/Log.h"

#include <limits>

#define super ds2::Target::POSIX::Process

namespace ds2 {
namespace Target {
namespace POSIX {

//
// ECOFF Process Implementation
//
// ECOFF (Extended Common Object File Format) was used on:
// - OSF/1 / Digital UNIX / Tru64
// - SGI IRIX
// - Ultrix
// - Sony NEWS-OS
//
// ECOFF is simpler than ELF and predates it. It was the standard
// format on MIPS-based Unix systems before ELF adoption.
//

ErrorCode ECOFFProcess::updateInfo() {
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  return kSuccess;
}

ErrorCode ECOFFProcess::getSharedLibraryInfoAddress(Address &address) {
  // ECOFF shared library support
  // Note: OSF/1 and IRIX use different mechanisms for shared library tracking
  // For now, return unsupported. This can be enhanced later with OSF/1-specific
  // dynamic loader introspection.

  if (_sharedLibraryInfoAddress.valid()) {
    address = _sharedLibraryInfoAddress;
    return kSuccess;
  }

  DS2LOG(Debug, "ECOFF shared library enumeration not yet implemented");
  return kErrorUnsupported;
}

ErrorCode ECOFFProcess::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {

  // ECOFF shared library enumeration
  // OSF/1 uses a different mechanism than ELF's DT_DEBUG
  // The dynamic loader maintains a list that can be accessed via
  // loader-specific interfaces

  // For now, return empty list. Full implementation would:
  // 1. Locate the dynamic loader's shared library list
  // 2. Walk the list structure
  // 3. Read each library's name and load address
  // 4. Call the callback for each library

  DS2LOG(Debug, "ECOFF shared library enumeration not yet implemented");
  return kSuccess; // Return success with empty list for now
}

} // namespace POSIX
} // namespace Target
} // namespace ds2
