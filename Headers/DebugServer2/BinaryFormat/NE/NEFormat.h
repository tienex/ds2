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

#include "DebugServer2/Types.h"
#include <cstdint>
#include <vector>

namespace ds2 {
namespace BinaryFormat {
namespace NE {

// NE (New Executable) Format
// Used by Windows 1.x/2.x/3.x, OS/2 1.x, and DOS4

// NE Header
struct NEHeader {
  uint16_t ne_magic;         // Magic number (0x454E = "NE")
  uint8_t ne_ver;            // Version number
  uint8_t ne_rev;            // Revision number
  uint16_t ne_enttab;        // Offset of Entry Table
  uint16_t ne_cbenttab;      // Number of bytes in Entry Table
  uint32_t ne_crc;           // Checksum of whole file
  uint16_t ne_flags;         // Flag word
  uint16_t ne_autodata;      // Automatic data segment number
  uint16_t ne_heap;          // Initial heap allocation
  uint16_t ne_stack;         // Initial stack allocation
  uint32_t ne_csip;          // Initial CS:IP setting
  uint32_t ne_sssp;          // Initial SS:SP setting
  uint16_t ne_cseg;          // Count of file segments
  uint16_t ne_cmod;          // Entries in Module Reference Table
  uint16_t ne_cbnrestab;     // Size of non-resident name table
  uint16_t ne_segtab;        // Offset of Segment Table
  uint16_t ne_rsrctab;       // Offset of Resource Table
  uint16_t ne_restab;        // Offset of resident name table
  uint16_t ne_modtab;        // Offset of Module Reference Table
  uint16_t ne_imptab;        // Offset of Imported Names Table
  uint32_t ne_nrestab;       // Offset of Non-resident Names Table
  uint16_t ne_cmovent;       // Count of movable entries
  uint16_t ne_align;         // Segment alignment shift count
  uint16_t ne_cres;          // Count of resource segments
  uint8_t ne_exetyp;         // Target Operating system
  uint8_t ne_flagsothers;    // Other .EXE flags
  uint16_t ne_pretthunks;    // Offset to return thunks
  uint16_t ne_psegrefbytes;  // Offset to segment ref. bytes
  uint16_t ne_swaparea;      // Minimum code swap area size
  uint16_t ne_expver;        // Expected Windows version number
};

// NE Segment Table Entry
struct NESegmentEntry {
  uint16_t offset;           // Logical-sector offset (<<alignment)
  uint16_t length;           // Length of segment in bytes
  uint16_t flags;            // Segment flags
  uint16_t minalloc;         // Minimum allocation size
};

// NE Entry Table Entry (fixed)
struct NEEntryFixed {
  uint8_t flags;             // Entry point flags
  uint16_t offset;           // Offset within segment
};

// NE Entry Table Entry (movable)
struct NEEntryMovable {
  uint8_t flags;             // Entry point flags
  uint16_t int3f;            // INT 3F instruction
  uint8_t segment;           // Segment number
  uint16_t offset;           // Offset within segment
};

// NE flags
enum NEFlags {
  NE_FFLAGS_SINGLEDATA = 0x0001,     // Single data
  NE_FFLAGS_MULTIPLEDATA = 0x0002,   // Multiple data
  NE_FFLAGS_SELFLOAD = 0x0800,       // Self-loading
  NE_FFLAGS_LINKERROR = 0x2000,      // Linker detected errors
  NE_FFLAGS_LIBMODULE = 0x8000,      // Library module
};

// NE target operating systems
enum NETargetOS {
  NE_OS_UNKNOWN = 0,
  NE_OS_OS2 = 1,                     // OS/2
  NE_OS_WINDOWS = 2,                 // Windows
  NE_OS_DOS4 = 3,                    // European MS-DOS 4.x
  NE_OS_WINDOWS386 = 4,              // Windows 386
  NE_OS_BOSS = 5,                    // Borland Operating System Services
};

// NE segment flags
enum NESegmentFlags {
  NE_SEG_DATA = 0x0001,              // Data segment
  NE_SEG_ALLOCATED = 0x0002,         // Allocated
  NE_SEG_LOADED = 0x0004,            // Loaded
  NE_SEG_MOVEABLE = 0x0010,          // Moveable
  NE_SEG_PURE = 0x0020,              // Pure (shareable)
  NE_SEG_PRELOAD = 0x0040,           // Preload
  NE_SEG_RELOC = 0x0100,             // Relocation info
  NE_SEG_DISCARD = 0x1000,           // Discardable
  NE_SEG_32BIT = 0x2000,             // 32-bit code segment
};

// Magic numbers
#define NE_MAGIC          0x454E       // "NE"

// Helper class for NE format parsing
class NEParser {
public:
  static bool isNE(const uint8_t *data, size_t size);
  static bool parseHeader(const uint8_t *data, size_t size, NEHeader &header);
  static uint32_t getEntryPoint(const NEHeader &header);
  static bool getSegments(const uint8_t *data, size_t size,
                         const NEHeader &header,
                         std::vector<NESegmentEntry> &segments);
  static bool isWindows(const NEHeader &header);
  static bool isOS2(const NEHeader &header);
  static bool isDOS4(const NEHeader &header);
};

} // namespace NE
} // namespace BinaryFormat
} // namespace ds2
