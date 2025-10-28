//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Cell Broadband Engine - Synergistic Processing Unit (SPU)
//

#pragma once

#include "DebugServer2/Base.h"

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace Cell {

//
// Cell SPU CPU State
//
// The SPU (Synergistic Processing Unit) is a specialized SIMD processor
// used in the Cell Broadband Engine (PlayStation 3, IBM Cell blades).
//
// SPU has a completely different ISA from PowerPC:
// - 128 general purpose registers (128-bit each)
// - All operations are SIMD (no scalar operations)
// - Local store instead of cache (256 KB)
// - Separate instruction set from PPU (PowerPC Processing Element)
//

struct SPUState {
  // General purpose registers (r0-r127, all 128-bit)
  // SPU treats all data as vectors
  struct {
    uint32_t word[4];  // 128-bit as 4 x 32-bit words
  } gpr[128];

  // Program counter
  uint32_t pc;       // SPU program counter (word address in local store)

  // Special purpose registers
  uint32_t npc;      // Next program counter

  // Channel registers (used for I/O and synchronization)
  // MFC (Memory Flow Controller) channels
  uint32_t mfc_cq[16];  // MFC command queue

  // Decrementer
  uint32_t decrementer;

  // Event masks and status
  uint32_t event_mask;
  uint32_t event_status;

  // Signal notification registers
  uint32_t signal1;
  uint32_t signal2;

  // Mailbox registers
  uint32_t mb_stat;    // Mailbox status
  uint32_t mb_data;    // Mailbox data

  // FPSCR (Floating-Point Status and Control Register)
  uint32_t fpscr;

  // SPU status
  uint32_t status;

  // Local store address
  uint32_t lslr;       // Local Store Limit Register

  inline void clear() { memset(this, 0, sizeof(*this)); }
};

} // namespace Cell
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
