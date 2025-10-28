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
namespace Atari {

// Atari TOS/GEMDOS Executable Format
// Used on Atari ST, STE, TT, Falcon, and MiNT/FreeMiNT
// 68000 architecture

// TOS Program Header
struct TOSHeader {
  uint16_t magic;             // Magic number (0x601A)
  uint32_t text_size;         // Size of text segment
  uint32_t data_size;         // Size of data segment
  uint32_t bss_size;          // Size of BSS segment
  uint32_t symbol_size;       // Size of symbol table
  uint32_t reserved1;         // Reserved
  uint32_t flags;             // Program flags
  uint16_t reloc_flag;        // Relocation flag (0 = relocate, 1 = no reloc)
};

// Extended TOS Header (for MiNT)
struct TOSExtendedHeader {
  uint32_t magic;             // Extended magic (0x4D694E54 = "MiNT")
  uint32_t info;              // Extended info flags
  uint32_t stack_size;        // Stack size
  uint32_t symbol_format;     // Symbol format
  uint32_t program_flags;     // Extended program flags
  uint16_t reserved[8];       // Reserved
};

// TOS Magic Numbers
static const uint16_t kTOSMagic = 0x601A;           // Standard TOS
static const uint16_t kTOSMagicPRG = 0x601A;        // .PRG files
static const uint16_t kTOSMagicTTP = 0x601A;        // .TTP files
static const uint16_t kTOSMagicTOS = 0x601A;        // .TOS files
static const uint16_t kTOSMagicAPP = 0x601A;        // .APP files (GEM)
static const uint32_t kTOSExtMagic = 0x4D694E54;   // "MiNT"

// Program Flags
enum TOSFlags {
  kTOSFlagFastLoad = 0x0001,      // Fast load (no clear)
  kTOSFlagTTRAMLoad = 0x0002,     // Load into TT-RAM
  kTOSFlagTTRAMAlloc = 0x0004,    // Allocate from TT-RAM
  kTOSFlagPrivate = 0x0008,       // Private memory
  kTOSFlagGlobal = 0x0010,        // Global memory
  kTOSFlagSuper = 0x0020,         // Super memory
  kTOSFlagReadable = 0x0040,      // Readable memory
  kTOSFlagShared = 0x0080,        // Shared memory
};

// Relocation entry
struct TOSRelocation {
  uint32_t offset;            // Offset to relocate
};

class TOSLoader {
public:
  TOSLoader();
  ~TOSLoader();

public:
  // Load TOS executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse TOS headers
  ErrorCode parseHeaders(void const *data, size_t size);

  // Get segments
  ErrorCode getSegments(Address &textAddr, uint32_t &textSize,
                       Address &dataAddr, uint32_t &dataSize,
                       Address &bssAddr, uint32_t &bssSize) const;

  // Check if has relocations
  bool hasRelocations() const { return _hasRelocations; }

  // Check if extended format (MiNT)
  bool isExtended() const { return _isExtended; }

private:
  TOSHeader _header;
  TOSExtendedHeader _extHeader;
  std::vector<TOSRelocation> _relocations;
  std::vector<uint8_t> _data;
  bool _parsed;
  bool _hasRelocations;
  bool _isExtended;

private:
  ErrorCode loadSegments(ProcessBase *process, Address baseAddress,
                        Address &textAddr, Address &dataAddr, Address &bssAddr);
  ErrorCode processRelocations(ProcessBase *process, Address baseAddress);
  ErrorCode parseRelocationTable();
};

} // namespace Atari
} // namespace Target
} // namespace ds2
