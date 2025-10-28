//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Architecture/MIPS/ABI.h"

#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace FreeBSD {

namespace {

template <typename T>
static inline void InitCodeVector(ByteVector &codestr, T const &init) {
  uint8_t const *bptr = reinterpret_cast<uint8_t const *>(&init);
  uint8_t const *eptr = bptr + sizeof init;
  codestr.assign(bptr, eptr);
}

// FreeBSD MIPS64 syscall injection (N64 ABI)
static uint32_t const gMIPS64MmapCode[] = {
    0x24020000,  // li   $v0, 0      # syscall number (patched)
    0x24040000,  // li   $a0, 0      # addr = NULL
    0x24050000,  // li   $a1, 0      # size (patched)
    0x24060000,  // li   $a2, 0      # prot (patched)
    0x24070000,  // li   $a3, 0      # flags (patched)
    0x2408ffff,  // li   $a4, -1     # fd = -1
    0x24090000,  // li   $a5, 0      # offset = 0
    0x0000000c,  // syscall
    0x0000000d,  // break 1
};

static uint32_t const gMIPS64MunmapCode[] = {
    0x24020000,  // li   $v0, 0      # syscall number (patched)
    0x24040000,  // li   $a0, 0      # addr (patched)
    0x24050000,  // li   $a1, 0      # size (patched)
    0x0000000c,  // syscall
    0x0000000d,  // break 1
};

static inline void MIPSSetLIImmediate(uint32_t *insn, uint16_t value) {
  *insn = (*insn & 0xffff0000) | value;
}

static void MIPSPrepareMmapCode(size_t size, int protection,
                                ByteVector &codestr) {
  // FreeBSD mmap syscall number is 477
  InitCodeVector(codestr, gMIPS64MmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, 477);                            // FreeBSD mmap
  MIPSSetLIImmediate(code + 2, size & 0xffff);                  // size
  MIPSSetLIImmediate(code + 3, protection & 0xffff);            // prot
  MIPSSetLIImmediate(code + 4, (MAP_ANON | MAP_PRIVATE) & 0xffff); // flags
}

static void MIPSPrepareMunmapCode(uint64_t address, size_t size,
                                  ByteVector &codestr) {
  // FreeBSD munmap syscall number is 73
  InitCodeVector(codestr, gMIPS64MunmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, 73);                // FreeBSD munmap
  MIPSSetLIImmediate(code + 1, address & 0xffff);  // addr (low 16 bits)
  MIPSSetLIImmediate(code + 2, size & 0xffff);     // size
}

} // namespace

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  Architecture::CPUState state;
  error = ptrace().readCPUState(ptid(), info, state);
  if (error != kSuccess)
    return error;

  ByteVector codestr;
  MIPSPrepareMmapCode(size, protection, codestr);

  uint64_t result = 0;
  error = ptrace().execute(ptid(), info, &codestr[0], codestr.size(), result);
  if (error != kSuccess) {
    ptrace().writeCPUState(ptid(), info, state);
    return error;
  }

  if ((int64_t)result < 0 && (int64_t)result >= -0x1000) {
    ptrace().writeCPUState(ptid(), info, state);
    return Platform::TranslateError(-result);
  }

  *address = result;

  return ptrace().writeCPUState(ptid(), info, state);
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  if (address == 0)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  Architecture::CPUState state;
  error = ptrace().readCPUState(ptid(), info, state);
  if (error != kSuccess)
    return error;

  ByteVector codestr;
  MIPSPrepareMunmapCode(address, size, codestr);

  uint64_t result = 0;
  error = ptrace().execute(ptid(), info, &codestr[0], codestr.size(), result);
  if (error != kSuccess) {
    ptrace().writeCPUState(ptid(), info, state);
    return error;
  }

  if ((int64_t)result < 0 && (int64_t)result >= -0x1000) {
    ptrace().writeCPUState(ptid(), info, state);
    return Platform::TranslateError(-result);
  }

  return ptrace().writeCPUState(ptid(), info, state);
}

} // namespace FreeBSD
} // namespace Target
} // namespace ds2
