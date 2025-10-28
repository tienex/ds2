//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

namespace ds2 {
namespace Architecture {
namespace Alpha {

enum RegisterIndex {
  // Integer registers
  kRegisterIndexR0 = 0,
  kRegisterIndexR1,
  kRegisterIndexR2,
  kRegisterIndexR3,
  kRegisterIndexR4,
  kRegisterIndexR5,
  kRegisterIndexR6,
  kRegisterIndexR7,
  kRegisterIndexR8,
  kRegisterIndexR9,
  kRegisterIndexR10,
  kRegisterIndexR11,
  kRegisterIndexR12,
  kRegisterIndexR13,
  kRegisterIndexR14,
  kRegisterIndexR15,
  kRegisterIndexR16,
  kRegisterIndexR17,
  kRegisterIndexR18,
  kRegisterIndexR19,
  kRegisterIndexR20,
  kRegisterIndexR21,
  kRegisterIndexR22,
  kRegisterIndexR23,
  kRegisterIndexR24,
  kRegisterIndexR25,
  kRegisterIndexR26,
  kRegisterIndexR27,
  kRegisterIndexR28,
  kRegisterIndexR29,
  kRegisterIndexR30,
  kRegisterIndexR31,

  // Floating-point registers
  kRegisterIndexF0 = 32,
  kRegisterIndexF1,
  kRegisterIndexF2,
  kRegisterIndexF3,
  kRegisterIndexF4,
  kRegisterIndexF5,
  kRegisterIndexF6,
  kRegisterIndexF7,
  kRegisterIndexF8,
  kRegisterIndexF9,
  kRegisterIndexF10,
  kRegisterIndexF11,
  kRegisterIndexF12,
  kRegisterIndexF13,
  kRegisterIndexF14,
  kRegisterIndexF15,
  kRegisterIndexF16,
  kRegisterIndexF17,
  kRegisterIndexF18,
  kRegisterIndexF19,
  kRegisterIndexF20,
  kRegisterIndexF21,
  kRegisterIndexF22,
  kRegisterIndexF23,
  kRegisterIndexF24,
  kRegisterIndexF25,
  kRegisterIndexF26,
  kRegisterIndexF27,
  kRegisterIndexF28,
  kRegisterIndexF29,
  kRegisterIndexF30,
  kRegisterIndexF31,

  // Special registers
  kRegisterIndexPC = 64,
  kRegisterIndexFPCR,
  kRegisterIndexUnique,

  kMaxRegisterIndex
};

// Register name aliases
enum {
  kRegisterIndexV0 = kRegisterIndexR0,
  kRegisterIndexT0 = kRegisterIndexR1,
  kRegisterIndexT1 = kRegisterIndexR2,
  kRegisterIndexT2 = kRegisterIndexR3,
  kRegisterIndexT3 = kRegisterIndexR4,
  kRegisterIndexT4 = kRegisterIndexR5,
  kRegisterIndexT5 = kRegisterIndexR6,
  kRegisterIndexT6 = kRegisterIndexR7,
  kRegisterIndexT7 = kRegisterIndexR8,
  kRegisterIndexS0 = kRegisterIndexR9,
  kRegisterIndexS1 = kRegisterIndexR10,
  kRegisterIndexS2 = kRegisterIndexR11,
  kRegisterIndexS3 = kRegisterIndexR12,
  kRegisterIndexS4 = kRegisterIndexR13,
  kRegisterIndexS5 = kRegisterIndexR14,
  kRegisterIndexFP = kRegisterIndexR15,
  kRegisterIndexA0 = kRegisterIndexR16,
  kRegisterIndexA1 = kRegisterIndexR17,
  kRegisterIndexA2 = kRegisterIndexR18,
  kRegisterIndexA3 = kRegisterIndexR19,
  kRegisterIndexA4 = kRegisterIndexR20,
  kRegisterIndexA5 = kRegisterIndexR21,
  kRegisterIndexT8 = kRegisterIndexR22,
  kRegisterIndexT9 = kRegisterIndexR23,
  kRegisterIndexT10 = kRegisterIndexR24,
  kRegisterIndexT11 = kRegisterIndexR25,
  kRegisterIndexRA = kRegisterIndexR26,
  kRegisterIndexPV = kRegisterIndexR27,
  kRegisterIndexAT = kRegisterIndexR28,
  kRegisterIndexGP = kRegisterIndexR29,
  kRegisterIndexSP = kRegisterIndexR30,
  kRegisterIndexZero = kRegisterIndexR31,
};

} // namespace Alpha
} // namespace Architecture
} // namespace ds2
