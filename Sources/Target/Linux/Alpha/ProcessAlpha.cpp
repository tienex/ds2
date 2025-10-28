//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Linux Alpha Process - Syscall injection
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

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

// Linux Alpha syscall numbers
static const int ALPHA_SYS_mmap = 71;
static const int ALPHA_SYS_munmap = 73;

// Alpha syscall code (same format as OSF/1)
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

static uint32_t const gAlphaMunmapCode[] = {
    0x201f0000,  // lda   v0, 0(zero)      # v0 = syscall number (patched)
    0x22100000,  // lda   a0, 0(zero)      # a0 = addr (patched low)
    0x26100000,  // ldah  a0, 0(a0)        # a0 high (patched)
    0x22310000,  // lda   a1, 0(zero)      # a1 = len (patched)
    0x00000083,  // callsys                # syscall
    0x00000080,  // bpt                    # breakpoint
};

static inline void AlphaSetLDAImmediate(uint32_t *insn, uint16_t value) {
  *insn = (*insn & 0xFFFF0000) | value;
}

static void AlphaPrepareMmapCode(size_t size, int protection, ByteVector &codestr) {
  InitCodeVector(codestr, gAlphaMmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  AlphaSetLDAImmediate(code + 0, ALPHA_SYS_mmap & 0xffff);
  AlphaSetLDAImmediate(code + 2, size & 0xffff);
  AlphaSetLDAImmediate(code + 3, protection & 0xffff);
  AlphaSetLDAImmediate(code + 4, (MAP_ANON | MAP_PRIVATE) & 0xffff);
}

static void AlphaPrepareMunmapCode(uint64_t address, size_t size, ByteVector &codestr) {
  InitCodeVector(codestr, gAlphaMunmapCode);
  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);

  AlphaSetLDAImmediate(code + 0, ALPHA_SYS_munmap & 0xffff);
  AlphaSetLDAImmediate(code + 1, address & 0xffff);
  AlphaSetLDAImmediate(code + 2, (address >> 16) & 0xffff);
  AlphaSetLDAImmediate(code + 3, size & 0xffff);
}

} // namespace

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr)
    return kErrorInvalidArgument;

  Architecture::CPUState state;
  CHK(ptrace().readCPUState(ptid(), _info, state));

  ByteVector codestr;
  AlphaPrepareMmapCode(size, protection, codestr);

  uint64_t pc = state.alpha.pc;
  CHK(execute(codestr, state));
  *address = state.alpha.gp.v0;

  state.alpha.pc = pc;
  return ptrace().writeCPUState(ptid(), _info, state);
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  Architecture::CPUState state;
  CHK(ptrace().readCPUState(ptid(), _info, state));

  ByteVector codestr;
  AlphaPrepareMunmapCode(address, size, codestr);

  uint64_t pc = state.alpha.pc;
  CHK(execute(codestr, state));

  state.alpha.pc = pc;
  return ptrace().writeCPUState(ptid(), _info, state);
}

} // namespace Linux
} // namespace Target
} // namespace ds2
