//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

// HP-UX uses SOM for PA-RISC, ELF for Itanium
#if defined(ARCH_PARISC) || defined(ARCH_PARISC64)
#include "DebugServer2/Target/POSIX/SOMProcess.h"
namespace ds2 { namespace Target { namespace HPUX {
class Process : public POSIX::SOMProcess {
#else
#include "DebugServer2/Target/POSIX/ELFProcess.h"
namespace ds2 { namespace Target { namespace HPUX {
class Process : public POSIX::ELFProcess {
#endif
public:
  Process();
  ~Process() override;

protected:
  ErrorCode updateInfo() override;
};

} // namespace HPUX
} // namespace Target
} // namespace ds2
