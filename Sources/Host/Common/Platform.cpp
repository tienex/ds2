//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {

ds2::CPUType Platform::GetCPUType() {
#if defined(ARCH_ARM) || defined(ARCH_ARM64)
  return (sizeof(void *) == 8) ? kCPUTypeARM64 : kCPUTypeARM;
#elif defined(ARCH_X86) || defined(ARCH_X86_64)
  return (sizeof(void *) == 8) ? kCPUTypeX86_64 : kCPUTypeI386;
#elif defined(ARCH_MIPS) || defined(ARCH_MIPS64)
  return (sizeof(void *) == 8) ? kCPUTypeMIPS64 : kCPUTypeMIPS;
#else
#error "Architecture not supported."
#endif
}

ds2::CPUSubType Platform::GetCPUSubType() {
#if defined(ARCH_ARM)
#if defined(__ARM_ARCH_7EM__)
  return kCPUSubTypeARM_V7EM;
#elif defined(__ARM_ARCH_7M__)
  return kCPUSubTypeARM_V7M;
#elif defined(OS_WIN32) || defined(__ARM_ARCH_7__) ||                          \
    defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__)
  return kCPUSubTypeARM_V7;
#endif
#endif // ARCH_ARM

#if defined(ARCH_MIPS) || defined(ARCH_MIPS64)
  // Detect MIPS architecture version based on compiler macros
#if defined(_MIPS_ARCH_MIPS64R6) || defined(__mips_isa_rev) && __mips_isa_rev >= 6
  return (sizeof(void *) == 8) ? kCPUSubTypeMIPS64_R6 : kCPUSubTypeMIPS32_R6;
#elif defined(_MIPS_ARCH_MIPS64R5) || defined(__mips_isa_rev) && __mips_isa_rev == 5
  return (sizeof(void *) == 8) ? kCPUSubTypeMIPS64_R5 : kCPUSubTypeMIPS32_R5;
#elif defined(_MIPS_ARCH_MIPS64R3) || defined(__mips_isa_rev) && __mips_isa_rev == 3
  return (sizeof(void *) == 8) ? kCPUSubTypeMIPS64_R3 : kCPUSubTypeMIPS32_R3;
#elif defined(_MIPS_ARCH_MIPS64R2) || defined(_MIPS_ARCH_MIPS32R2) || \
    (defined(__mips_isa_rev) && __mips_isa_rev == 2)
  return (sizeof(void *) == 8) ? kCPUSubTypeMIPS64_R2 : kCPUSubTypeMIPS32_R2;
#elif defined(_MIPS_ARCH_MIPS64) || defined(_MIPS_ARCH_MIPS32) || \
    (defined(__mips_isa_rev) && __mips_isa_rev == 1)
  return (sizeof(void *) == 8) ? kCPUSubTypeMIPS64_R1 : kCPUSubTypeMIPS32_R1;
#elif defined(_MIPS_ARCH_MIPS5) || (defined(_MIPS_ISA) && _MIPS_ISA == _MIPS_ISA_MIPS5)
  return kCPUSubTypeMIPS_R16000;
#elif defined(_MIPS_ARCH_MIPS4) || (defined(_MIPS_ISA) && _MIPS_ISA == _MIPS_ISA_MIPS4)
  return kCPUSubTypeMIPS_R10000;
#elif defined(_MIPS_ARCH_MIPS3) || (defined(_MIPS_ISA) && _MIPS_ISA == _MIPS_ISA_MIPS3)
  return kCPUSubTypeMIPS_R4000;
#elif defined(_MIPS_ARCH_MIPS2) || (defined(_MIPS_ISA) && _MIPS_ISA == _MIPS_ISA_MIPS2)
  return kCPUSubTypeMIPS_R6000;
#elif defined(_MIPS_ARCH_MIPS1) || (defined(_MIPS_ISA) && _MIPS_ISA == _MIPS_ISA_MIPS1)
  return kCPUSubTypeMIPS_R3000;
#else
  return kCPUSubTypeMIPS_ALL;
#endif
#endif // ARCH_MIPS

  return kCPUSubTypeInvalid;
}

ds2::Endian Platform::GetEndian() {
#if defined(ENDIAN_LITTLE)
  return kEndianLittle;
#elif defined(ENDIAN_BIG)
  return kEndianBig;
#elif defined(ENDIAN_MIDDLE)
  return kEndianPDP;
#else
  return kEndianUnknown;
#endif
}

size_t Platform::GetPointerSize() { return sizeof(void *); }
} // namespace Host
} // namespace ds2
