//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 Alpha Process - Syscall injection for memory allocation
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>
#include <sys/mman.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace OSF1 {

namespace {

template <typename T>
static inline void InitCodeVector(ByteVector &codestr, T const &init) {
  uint8_t const *bptr = reinterpret_cast<uint8_t const *>(&init);
  uint8_t const *eptr = bptr + sizeof init;
  codestr.assign(bptr, eptr);
}

// OSF/1 syscall numbers for Alpha
// Note: OSF/1 uses BSD-style syscall numbers
static const int OSF1_SYS_mmap = 71;
static const int OSF1_SYS_munmap = 73;

//
// Alpha syscall injection code
//
// Alpha calling convention:
//   Arguments: a0-a5 (r16-r21) for first 6 args
//   Syscall number: v0 (r0)
//   Result: v0 (r0)
//   Syscall instruction: callsys (0x83)
//
// Alpha instructions are 32-bit little-endian
//

// mmap syscall for Alpha
// mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
static uint32_t const gAlphaMmapCode[] = {
    0x201f0000,  // lda   v0, 0(zero)      # v0 = syscall number (patched)
    0x22100000,  // lda   a0, 0(zero)      # a0 = addr = NULL
    0x22310000,  // lda   a1, 0(zero)      # a1 = len (patched)
    0x22520000,  // lda   a2, 0(zero)      # a2 = prot (patched)
    0x22730000,  // lda   a3, 0(zero)      # a3 = flags (patched)
    0x2294ffff,  // lda   a4, -1(zero)     # a4 = fd = -1
    0x22b50000,  // lda   a5, 0(zero)      # a5 = offset = 0
    0x00000083,  // callsys                # syscall
    0x00000080,  // bpt                    # breakpoint
};

// munmap syscall for Alpha
// munmap(void *addr, size_t len)
static uint32_t const gAlphaMunmapCode[] = {
    0x201f0000,  // lda   v0, 0(zero)      # v0 = syscall number (patched)
    0x22100000,  // lda   a0, 0(zero)      # a0 = addr (patched low 16 bits)
    0x26100000,  // ldah  a0, 0(a0)        # a0 high 16 bits (patched)
    0x22310000,  // lda   a1, 0(zero)      # a1 = len (patched)
    0x00000083,  // callsys                # syscall
    0x00000080,  // bpt                    # breakpoint
};

// Patch lda instruction immediate (low 16 bits)
static inline void AlphaSetLDAImmediate(uint32_t *insn, uint16_t value) {
  *insn = (*insn & 0xFFFF0000) | value;
}

// Patch ldah instruction immediate (high 16 bits)
static inline void AlphaSetLDAHImmediate(uint32_t *insn, uint16_t value) {
  *insn = (*insn & 0xFFFF0000) | value;
}

static void AlphaPrepareMmapCode(size_t size, int protection,
                                ByteVector &codestr) {
  InitCodeVector(codestr, gAlphaMmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  AlphaSetLDAImmediate(code + 0, OSF1_SYS_mmap & 0xffff);        // syscall number
  AlphaSetLDAImmediate(code + 2, size & 0xffff);                 // size (low 16)
  AlphaSetLDAImmediate(code + 3, protection & 0xffff);           // prot
  AlphaSetLDAImmediate(code + 4, (MAP_ANON | MAP_PRIVATE) & 0xffff); // flags
}

static void AlphaPrepareMunmapCode(uint64_t address, size_t size,
                                  ByteVector &codestr) {
  InitCodeVector(codestr, gAlphaMunmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  uint16_t addr_low = address & 0xffff;
  uint16_t addr_high = (address >> 16) & 0xffff;

  AlphaSetLDAImmediate(code + 0, OSF1_SYS_munmap & 0xffff);  // syscall number
  AlphaSetLDAImmediate(code + 1, addr_low);                  // addr (low 16)
  AlphaSetLDAHImmediate(code + 2, addr_high);                // addr (high 16)
  AlphaSetLDAImmediate(code + 3, size & 0xffff);             // size
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
  error = readCPUState(_currentThread->tid(), state);
  if (error != kSuccess)
    return error;

  ByteVector codestr;
  AlphaPrepareMmapCode(size, protection, codestr);

  // Save current PC
  uint64_t pc = state.alpha.pc;

  // Execute the code
  error = execute(codestr, state);
  if (error != kSuccess)
    return error;

  // Get result from v0 (r0)
  *address = state.alpha.gp.v0;

  // Restore PC
  state.alpha.pc = pc;
  error = writeCPUState(_currentThread->tid(), state);

  return error;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  Architecture::CPUState state;
  error = readCPUState(_currentThread->tid(), state);
  if (error != kSuccess)
    return error;

  ByteVector codestr;
  AlphaPrepareMunmapCode(address, size, codestr);

  // Save current PC
  uint64_t pc = state.alpha.pc;

  // Execute the code
  error = execute(codestr, state);
  if (error != kSuccess)
    return error;

  // Restore PC
  state.alpha.pc = pc;
  error = writeCPUState(_currentThread->tid(), state);

  return error;
}

} // namespace OSF1
} // namespace Target
} // namespace ds2
