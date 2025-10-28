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
namespace Linux {

namespace {

template <typename T>
static inline void InitCodeVector(ByteVector &codestr, T const &init) {
  uint8_t const *bptr = reinterpret_cast<uint8_t const *>(&init);
  uint8_t const *eptr = bptr + sizeof init;
  codestr.assign(bptr, eptr);
}

// Detect MIPS ABI at runtime based on ELF flags
static Architecture::MIPS::ABI DetectProcessABI() {
  // For MIPS64, default to N64 unless N32 is detected
  // In a full implementation, we would read the ELF header of the target process
  return Architecture::MIPS::DetectABI();
}

// Get syscall number based on ABI
static int GetSyscallNumber(Architecture::MIPS::ABI abi, const char *name) {
  // Syscall numbers vary by ABI
  // N32: mmap=6009, munmap=6011
  // N64: mmap=5009, munmap=5011

  if (strcmp(name, "mmap") == 0) {
    switch (abi) {
    case Architecture::MIPS::ABI::N32:
      return 6009;
    case Architecture::MIPS::ABI::N64:
      return 5009;
    default:
      return __NR_mmap; // fallback
    }
  } else if (strcmp(name, "munmap") == 0) {
    switch (abi) {
    case Architecture::MIPS::ABI::N32:
      return 6011;
    case Architecture::MIPS::ABI::N64:
      return 5011;
    default:
      return __NR_munmap; // fallback
    }
  }
  return -1;
}

//
// MIPS64 syscall injection code - N32/N64 ABI
// Both N32 and N64: a0-a7 for first 8 args, syscall number in v0
//

// mmap syscall for MIPS64 (N32/N64)
// All 6 args fit in registers
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

// munmap syscall for MIPS64
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
  Architecture::MIPS::ABI abi = DetectProcessABI();
  int syscall_nr = GetSyscallNumber(abi, "mmap");

  InitCodeVector(codestr, gMIPS64MmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, syscall_nr & 0xffff);        // syscall number
  MIPSSetLIImmediate(code + 2, size & 0xffff);              // size
  MIPSSetLIImmediate(code + 3, protection & 0xffff);        // prot
  MIPSSetLIImmediate(code + 4, (MAP_ANON | MAP_PRIVATE) & 0xffff); // flags
}

static void MIPSPrepareMunmapCode(uint64_t address, size_t size,
                                  ByteVector &codestr) {
  Architecture::MIPS::ABI abi = DetectProcessABI();
  int syscall_nr = GetSyscallNumber(abi, "munmap");

  InitCodeVector(codestr, gMIPS64MunmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  MIPSSetLIImmediate(code + 0, syscall_nr & 0xffff);  // syscall number
  MIPSSetLIImmediate(code + 1, address & 0xffff);     // addr
  MIPSSetLIImmediate(code + 2, size & 0xffff);        // size
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

  //
  // Restore CPU state
  //
  return ptrace().writeCPUState(ptid(), info, state);
}

} // namespace Linux
} // namespace Target
} // namespace ds2
