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

#include "DebugServer2/Target/ProcessBase.h"
#include "DebugServer2/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ds2 {
namespace Target {
namespace Amiga {

// Amiga Hunk Executable Format
// Used on AmigaOS (68k, PowerPC, x86)
// Format consists of hunks (segments) with relocations

// Hunk Types
enum HunkType {
  kHunkUnit = 0x3E7,          // 999 - Object file unit
  kHunkName = 0x3E8,          // 1000 - Name hunk
  kHunkCode = 0x3E9,          // 1001 - Code hunk
  kHunkData = 0x3EA,          // 1002 - Data hunk
  kHunkBSS = 0x3EB,           // 1003 - BSS hunk
  kHunkReloc32 = 0x3EC,       // 1004 - 32-bit relocation
  kHunkReloc16 = 0x3ED,       // 1005 - 16-bit relocation
  kHunkReloc8 = 0x3EE,        // 1006 - 8-bit relocation
  kHunkExt = 0x3EF,           // 1007 - External symbols
  kHunkSymbol = 0x3F0,        // 1008 - Symbol table
  kHunkDebug = 0x3F1,         // 1009 - Debug info
  kHunkEnd = 0x3F2,           // 1010 - End of hunk
  kHunkHeader = 0x3F3,        // 1011 - Load file header
  kHunkOverlay = 0x3F5,       // 1013 - Overlay hunk
  kHunkBreak = 0x3F6,         // 1014 - Break hunk
  kHunkDReloc32 = 0x3F7,      // 1015 - 32-bit data relocation
  kHunkDReloc16 = 0x3F8,      // 1016 - 16-bit data relocation
  kHunkDReloc8 = 0x3F9,       // 1017 - 8-bit data relocation
  kHunkLibrary = 0x3FA,       // 1018 - Library base
  kHunkIndex = 0x3FB,         // 1019 - Index hunk
  kHunkReloc32Short = 0x3FC,  // 1020 - Short 32-bit relocation
  kHunkRelReloc32 = 0x3FD,    // 1021 - PC-relative 32-bit
  kHunkAbsReloc16 = 0x3FE,    // 1022 - Absolute 16-bit relocation
};

// Hunk Flags
enum HunkFlags {
  kHunkFlagChip = 0x40000000,     // CHIP memory
  kHunkFlagFast = 0x80000000,     // FAST memory
  kHunkFlagAdvisory = 0xC0000000, // Advisory memory
};

// Memory Types
enum MemoryType {
  kMemTypeAny = 0,            // Any memory
  kMemTypeChip = 1,           // Chip memory (graphics accessible)
  kMemTypeFast = 2,           // Fast memory
};

// Hunk structure
struct Hunk {
  uint32_t type;              // Hunk type
  uint32_t size;              // Size in longwords
  uint32_t memType;           // Memory type
  std::vector<uint8_t> data;  // Hunk data
  Address loadAddress;        // Load address (filled during load)
};

// Relocation entry
struct HunkRelocation {
  uint32_t targetHunk;        // Target hunk number
  std::vector<uint32_t> offsets; // Offsets to relocate
};

class HunkLoader {
public:
  HunkLoader();
  ~HunkLoader();

public:
  // Load Amiga Hunk executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse hunks
  ErrorCode parseHunks(void const *data, size_t size);

  // Get hunks
  ErrorCode getHunks(std::vector<Hunk> &hunks) const;

  // Check memory type requirements
  bool requiresChipMem() const;
  bool requiresFastMem() const;

private:
  std::vector<Hunk> _hunks;
  std::vector<std::vector<HunkRelocation>> _relocations;
  std::vector<uint8_t> _data;
  bool _parsed;

private:
  ErrorCode loadHunk(ProcessBase *process, size_t hunkIndex,
                    Address baseAddress);
  ErrorCode processHunkRelocations(ProcessBase *process, size_t hunkIndex);
  
  // Read big-endian 32-bit value
  uint32_t readBE32(const uint8_t *&ptr);
  
  // Read string (Pascal-style)
  std::string readString(const uint8_t *&ptr);
};

} // namespace Amiga
} // namespace Target
} // namespace ds2
