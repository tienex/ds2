//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Types.h"

namespace ds2 {

char const *GetCPUTypeName(CPUType type) {
  switch (type) {
  case kCPUTypeAny:
    return "ANY";
  case kCPUTypeVAX:
    return "VAX";
  case kCPUTypeROMP:
    return "ROMP";
  case kCPUTypeMC680x0:
    return "M68K";
  case kCPUTypeI386:
    return "I386";
  case kCPUTypeX86_64:
    return "X86-64";
  case kCPUTypeMIPS:
    return "MIPS";
  case kCPUTypeMIPS64:
    return "MIPS64";
  case kCPUTypeMC98000:
    return "POWERPC";
  case kCPUTypeHPPA:
    return "HPPA";
  case kCPUTypeHPPA64:
    return "HPPA64";
  case kCPUTypeARM:
    return "ARM";
  case kCPUTypeARM64:
    return "ARM64";
  case kCPUTypeMC88000:
    return "M88K";
  case kCPUTypeSPARC:
    return "SPARC";
  case kCPUTypeSPARC64:
    return "SPARC64";
  case kCPUTypeI860:
    return "I860";
  case kCPUTypeALPHA:
    return "ALPHA";
  case kCPUTypePOWERPC:
    return "POWERPC";
  case kCPUTypePOWERPC64:
    return "POWERPC64";
  default:
    break;
  }
  return "UNKNOWN";
}

char const *GetArchName(CPUType type, CPUSubType subtype) {
  switch (type) {
  case kCPUTypeAny:
    return "any";
  case kCPUTypeVAX:
    return "vax";
  case kCPUTypeROMP:
    return "romp";
  case kCPUTypeMC680x0:
    return "m68k";
  case kCPUTypeI386:
    return "i386";
  case kCPUTypeX86_64:
    return "x86_64";
  case kCPUTypeMIPS:
    return "mips";
  case kCPUTypeMIPS64:
    return "mips64";
  case kCPUTypeMC98000:
    return "powerpc"; // Really, PowerPC 601
  case kCPUTypeHPPA:
    return "parisc";
  case kCPUTypeHPPA64:
    return "parisc64";
  case kCPUTypeARM:
    switch (subtype) {
    case kCPUSubTypeARM_V7:
    case kCPUSubTypeARM_V7EM:
    case kCPUSubTypeARM_V7F:
    case kCPUSubTypeARM_V7K:
    case kCPUSubTypeARM_V7M:
    case kCPUSubTypeARM_V7S:
      return "armv7";
    default:
      return "arm";
    }
  case kCPUTypeARM64:
    return "aarch64"; // arm64? armv8?
  case kCPUTypeMC88000:
    return "m88k";
  case kCPUTypeSPARC:
    return "sparc";
  case kCPUTypeSPARC64:
    return "sparc64";
  case kCPUTypeI860:
    return "i860";
  case kCPUTypeALPHA:
    return "alpha";
  case kCPUTypePOWERPC:
    return "powerpc";
  case kCPUTypePOWERPC64:
    return "powerpc64";
  default:
    break;
  }
  return "unknown";
}

char const *GetArchName(CPUType type, CPUSubType subtype, Endian endian) {
  switch (type) {
  case kCPUTypeARM:
    if (endian == kEndianLittle) {
      switch (subtype) {
      case kCPUSubTypeARM_V7:
      case kCPUSubTypeARM_V7F:
      case kCPUSubTypeARM_V7S:
      case kCPUSubTypeARM_V7K:
      case kCPUSubTypeARM_V7M:
      case kCPUSubTypeARM_V7EM:
        return "armv7";
      default:
        return "arm";
      }
    } else {
      return "armeb";
    }
  case kCPUTypePOWERPC:
    if (endian == kEndianLittle)
      return "powerpcle";
    else
      return "powerpc";
  case kCPUTypePOWERPC64:
    if (endian == kEndianLittle)
      return "powerpc64le";
    else
      return "powerpc64";
  case kCPUTypeMIPS:
    if (endian == kEndianLittle)
      return "mipsel";
    else
      return "mipseb";
  case kCPUTypeMIPS64:
    if (endian == kEndianLittle)
      return "mips64el";
    else
      return "mips64eb";
  default:
    break;
  }
  return GetArchName(type, subtype);
}

char const *GetCPUFeatureName(CPUFeatureFlags feature) {
  switch (feature) {
  // MIPS features
  case kCPUFeatureMIPS16:
    return "MIPS16e";
  case kCPUFeatureMicroMIPS:
    return "microMIPS";
  case kCPUFeatureMIPS_DSP:
    return "DSP ASE";
  case kCPUFeatureMIPS_DSP2:
    return "DSP ASE R2";
  case kCPUFeatureMIPS_DSP3:
    return "DSP ASE R3";
  case kCPUFeatureMIPS_MSA:
    return "MSA";
  case kCPUFeatureMIPS_MDMX:
    return "MDMX";
  case kCPUFeatureMIPS_3D:
    return "MIPS-3D";
  case kCPUFeatureMIPS_MT:
    return "Multi-Threading";
  case kCPUFeatureMIPS_VZ:
    return "Virtualization";
  case kCPUFeatureMIPS_EVA:
    return "EVA";
  case kCPUFeatureMIPS_SmartMIPS:
    return "SmartMIPS";
  case kCPUFeatureMIPS_PairedSingle:
    return "Paired-Single";
  case kCPUFeatureMIPS_CRC32:
    return "CRC32";
  case kCPUFeatureMIPS_GINV:
    return "GINV";

  // ARM features
  case kCPUFeatureARM_Thumb:
    return "Thumb";
  case kCPUFeatureARM_Thumb2:
    return "Thumb-2";
  case kCPUFeatureARM_NEON:
    return "NEON";
  case kCPUFeatureARM_VFP:
    return "VFP";

  default:
    break;
  }
  return "Unknown";
}
} // namespace ds2
