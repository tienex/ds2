//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

// NOTE: This file should be auto-generated from Definitions/MIPS64.json using RegsGen2
// This is a minimal stub implementation to allow compilation
// TODO: Generate full descriptors using: RegsGen2 Definitions/MIPS64.json

#include "DebugServer2/Architecture/MIPS64/RegistersDescriptors.h"

namespace ds2 {
namespace Architecture {
namespace MIPS64 {

static GDBFeature const *sGDBFeatures[] = {
  nullptr
};

static GDBDescriptor sGDBDescriptor = {
  "mips64",  // architecture
  nullptr,   // OSABI
  0,         // feature count
  sGDBFeatures
};

static LLDBRegisterSet const *sLLDBRegisterSets[] = {
  nullptr
};

static LLDBDescriptor sLLDBDescriptor = {
  0, // set count
  sLLDBRegisterSets
};

GDBDescriptor const &GetGDBDescriptor() {
  return sGDBDescriptor;
}

LLDBDescriptor const &GetLLDBDescriptor() {
  return sLLDBDescriptor;
}

} // namespace MIPS64
} // namespace Architecture
} // namespace ds2
