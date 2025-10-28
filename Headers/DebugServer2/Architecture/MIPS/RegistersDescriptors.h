//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

// NOTE: This file should be auto-generated from Definitions/MIPS32.json using RegsGen2
// This is a minimal stub to allow compilation

#pragma once

#include "DebugServer2/Architecture/RegisterLayout.h"

namespace ds2 {
namespace Architecture {
namespace MIPS {

//
// Register Numbers (GDB)
//
enum {
  reg_gdb_r0 = 0,
  reg_gdb_r31 = 31,
  reg_gdb_status = 32,
  reg_gdb_lo = 33,
  reg_gdb_hi = 34,
  reg_gdb_badvaddr = 35,
  reg_gdb_cause = 36,
  reg_gdb_pc = 37,
  reg_gdb_f0 = 38,
  reg_gdb_f31 = 69,
  reg_gdb_fcsr = 70,
  reg_gdb_fir = 71,
};

//
// Register Numbers (LLDB)
//
enum {
  reg_lldb_r0 = 0,
  reg_lldb_r31 = 31,
  reg_lldb_lo = 32,
  reg_lldb_hi = 33,
  reg_lldb_pc = 34,
  reg_lldb_status = 35,
  reg_lldb_badvaddr = 36,
  reg_lldb_cause = 37,
  reg_lldb_f0 = 38,
  reg_lldb_f31 = 69,
  reg_lldb_fcsr = 70,
  reg_lldb_fir = 71,
};

extern GDBDescriptor const &GetGDBDescriptor();
extern LLDBDescriptor const &GetLLDBDescriptor();

// Exported descriptors (for compatibility with ProcessBase)
extern GDBDescriptor const GDB;
extern LLDBDescriptor const LLDB;

} // namespace MIPS
} // namespace Architecture
} // namespace ds2
