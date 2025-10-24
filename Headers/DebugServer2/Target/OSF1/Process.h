//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 (Digital UNIX / Tru64) Process - Mach microkernel with ECOFF binaries
//

#pragma once

#include "DebugServer2/Host/OSF1/Mach.h"
#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Target/POSIX/ECOFFProcess.h"

namespace ds2 {
namespace Target {
namespace OSF1 {

class Process : public POSIX::ECOFFProcess {
protected:
  Host::POSIX::PTrace _ptrace;
  Host::OSF1::Mach _mach;

protected:
  ErrorCode attach(int waitStatus) override;

public:
  ErrorCode terminate() override;

public:
  ErrorCode suspend() override;

public:
  ErrorCode getMemoryRegionInfo(Address const &address,
                                MemoryRegionInfo &info) override;

public:
  ErrorCode readString(Address const &address, std::string &str, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *count = nullptr) override;

public:
  ErrorCode wait() override;

public:
  Host::POSIX::PTrace &ptrace() const override;
  Host::OSF1::Mach &mach() const;

protected:
  ErrorCode updateInfo() override;
  ErrorCode updateAuxiliaryVector() override;

protected:
  friend class Thread;
  ErrorCode readCPUState(ThreadId tid, Architecture::CPUState &state,
                         uint32_t flags = 0);
  ErrorCode writeCPUState(ThreadId tid, Architecture::CPUState const &state,
                          uint32_t flags = 0);

public:
  ErrorCode afterResume() override;

protected:
  friend class POSIX::Process;
};

} // namespace OSF1
} // namespace Target
} // namespace ds2
