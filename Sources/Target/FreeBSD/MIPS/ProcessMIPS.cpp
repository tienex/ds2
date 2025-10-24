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

// FreeBSD MIPS syscall injection for O32 ABI
// FreeBSD uses different syscall numbers than Linux
static uint32_t const gMIPSO32MmapCode[] = {
    0x27bdffe0,  // addiu $sp, $sp, -32  # allocate stack
    0xafbf001c,  // sw    $ra, 28($sp)   # save return address
    0x24020000,  // li    $v0, 0         # syscall number (patched)
    0x24040000,  // li    $a0, 0         # addr = NULL
    0x24050000,  // li    $a1, 0         # size (patched)
    0x24060000,  // li    $a2, 0         # prot (patched)
    0x24070000,  // li    $a3, 0         # flags (patched)
    0x2408ffff,  // li    $t0, -1        # fd = -1
    0xafa80010,  // sw    $t0, 16($sp)   # store fd on stack (arg5)
    0x24090000,  // li    $t1, 0         # offset = 0
    0xafa90014,  // sw    $t1, 20($sp)   # store offset on stack (arg6)
    0x0000000c,  // syscall
    0x8fbf001c,  // lw    $ra, 28($sp)   # restore return address
    0x27bd0020,  // addiu $sp, $sp, 32   # deallocate stack
    0x0000000d,  // break 1
};

static uint32_t const gMIPSO32MunmapCode[] = {
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
  InitCodeVector(codestr, gMIPSO32MmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 2, 477);                            // FreeBSD mmap
  MIPSSetLIImmediate(code + 4, size & 0xffff);                  // size
  MIPSSetLIImmediate(code + 5, protection & 0xffff);            // prot
  MIPSSetLIImmediate(code + 6, (MAP_ANON | MAP_PRIVATE) & 0xffff); // flags
}

static void MIPSPrepareMunmapCode(uint32_t address, size_t size,
                                  ByteVector &codestr) {
  // FreeBSD munmap syscall number is 73
  InitCodeVector(codestr, gMIPSO32MunmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, 73);                // FreeBSD munmap
  MIPSSetLIImmediate(code + 1, address & 0xffff);  // addr
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

  return ptrace().writeCPUState(ptid(), info, state);
}

} // namespace FreeBSD
} // namespace Target
} // namespace ds2
