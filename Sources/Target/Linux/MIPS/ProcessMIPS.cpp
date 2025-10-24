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

#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Linux {

namespace {

template <typename T>
static inline void InitCodeVector(ByteVector &codestr, T const &init) {
  uint8_t const *bptr = reinterpret_cast<uint8_t const *>(&init);
  uint8_t const *eptr = bptr + sizeof init;
  codestr.assign(bptr, eptr);
}

//
// MIPS syscall injection code
// MIPS calling convention: a0-a3 for first 4 args, syscall number in v0
//

// mmap syscall for MIPS
// li   $v0, __NR_mmap      # syscall number
// li   $a0, 0              # addr = NULL
// li   $a1, size           # length
// li   $a2, prot           # prot
// li   $a3, flags          # flags
// li   $a4, -1             # fd = -1
// li   $a5, 0              # offset = 0
// syscall
// break 1                  # trap to debugger

static uint32_t const gMIPSMmapCode[] = {
    0x24020000,  // li   $v0, 0x0000 (will be patched with __NR_mmap)
    0x24040000,  // li   $a0, 0      (addr = NULL)
    0x24050000,  // li   $a1, 0x0000 (will be patched with size)
    0x24060000,  // li   $a2, 0x0000 (will be patched with prot)
    0x24070000,  // li   $a3, 0x0000 (will be patched with flags)
    0x2408ffff,  // li   $t0, -1     (fd = -1)
    0x24090000,  // li   $t1, 0      (offset = 0)
    0x0000000c,  // syscall
    0x0000000d,  // break 1
};

// munmap syscall for MIPS
// li   $v0, __NR_munmap
// li   $a0, addr
// li   $a1, size
// syscall
// break 1

static uint32_t const gMIPSMunmapCode[] = {
    0x24020000,  // li   $v0, 0x0000 (will be patched with __NR_munmap)
    0x24040000,  // li   $a0, 0x0000 (will be patched with addr)
    0x24050000,  // li   $a1, 0x0000 (will be patched with size)
    0x0000000c,  // syscall
    0x0000000d,  // break 1
};

static inline void MIPSSetLIImmediate(uint32_t *insn, uint16_t value) {
  *insn = (*insn & 0xffff0000) | value;
}

static void MIPSPrepareMmapCode(size_t size, int protection,
                                ByteVector &codestr) {
  InitCodeVector(codestr, gMIPSMmapCode);

  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, __NR_mmap);
  MIPSSetLIImmediate(code + 2, size & 0xffff);
  MIPSSetLIImmediate(code + 3, protection & 0xffff);
  MIPSSetLIImmediate(code + 4, (MAP_ANON | MAP_PRIVATE) & 0xffff);
}

static void MIPSPrepareMunmapCode(uint32_t address, size_t size,
                                  ByteVector &codestr) {
  InitCodeVector(codestr, gMIPSMunmapCode);

  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, __NR_munmap);
  MIPSSetLIImmediate(code + 1, address & 0xffff);
  MIPSSetLIImmediate(code + 2, size & 0xffff);
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

  //
  // Prepare mmap code
  //
  ByteVector codestr;
  MIPSPrepareMmapCode(size, protection, codestr);

  //
  // Code inject and execute
  //
  uint32_t result = 0;
  error = ptrace().execute(ptid(), info, &codestr[0], codestr.size(), result);
  if (error != kSuccess) {
    ptrace().writeCPUState(ptid(), info, state);
    return error;
  }

  if ((int32_t)result < 0 && (int32_t)result >= -0x1000) {
    ptrace().writeCPUState(ptid(), info, state);
    return Platform::TranslateError(-result);
  }

  *address = result;

  //
  // Restore CPU state
  //
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

  //
  // Prepare munmap code
  //
  ByteVector codestr;
  MIPSPrepareMunmapCode(address, size, codestr);

  //
  // Code inject and execute
  //
  uint32_t result = 0;
  error = ptrace().execute(ptid(), info, &codestr[0], codestr.size(), result);
  if (error != kSuccess) {
    ptrace().writeCPUState(ptid(), info, state);
    return error;
  }

  if ((int32_t)result < 0 && (int32_t)result >= -0x1000) {
    ptrace().writeCPUState(ptid(), info, state);
    return Platform::TranslateError(-result);
  }

  //
  // Restore CPU state
  //
  return ptrace().writeCPUState(ptid(), info, state);
}

} // namespace Linux
} // namespace Target
} // namespace ds2
