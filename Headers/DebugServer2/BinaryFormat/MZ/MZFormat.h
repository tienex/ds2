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

namespace ds2 {
namespace BinaryFormat {
namespace MZ {

// MZ (DOS) Executable Format
// Used by MS-DOS, DOS/4G, DOS/4GW (DOS Extenders), and early Windows

// MZ Header (DOS EXE header)
struct MZHeader {
  uint16_t e_magic;          // Magic number (0x5A4D = "MZ")
  uint16_t e_cblp;           // Bytes on last page
  uint16_t e_cp;             // Pages in file
  uint16_t e_crlc;           // Relocations
  uint16_t e_cparhdr;        // Size of header in paragraphs
  uint16_t e_minalloc;       // Minimum extra paragraphs needed
  uint16_t e_maxalloc;       // Maximum extra paragraphs needed
  uint16_t e_ss;             // Initial (relative) SS value
  uint16_t e_sp;             // Initial SP value
  uint16_t e_csum;           // Checksum
  uint16_t e_ip;             // Initial IP value
  uint16_t e_cs;             // Initial (relative) CS value
  uint16_t e_lfarlc;         // File address of relocation table
  uint16_t e_ovno;           // Overlay number
  uint16_t e_res[4];         // Reserved words
  uint16_t e_oemid;          // OEM identifier
  uint16_t e_oeminfo;        // OEM information
  uint16_t e_res2[10];       // Reserved words
  uint32_t e_lfanew;         // File address of new exe header (PE/NE)
};

// MZ Relocation entry
struct MZRelocation {
  uint16_t offset;           // Offset within segment
  uint16_t segment;          // Segment
};

// DOS Extended formats (DOS/4G, DOS/4GW, DOS32)
struct DOS4GWHeader {
  uint16_t signature;        // 0x4C45 ("LE") or 0x584C ("LX")
  uint8_t byte_order;        // Byte order (0=little-endian)
  uint8_t word_order;        // Word order
  uint32_t format_level;     // Executable format level
  uint16_t cpu_type;         // CPU type (1=286, 2=386, 3=486)
  uint16_t os_type;          // Target OS (1=OS/2, 2=Windows, 3=DOS4)
  uint32_t module_version;   // Module version
  uint32_t module_flags;     // Module type flags
  uint32_t num_pages;        // Number of memory pages
  uint32_t eip_object;       // Object number for EIP
  uint32_t eip;              // Initial EIP value
  uint32_t esp_object;       // Object number for ESP
  uint32_t esp;              // Initial ESP value
  uint32_t page_size;        // Memory page size
  uint32_t bytes_on_last;    // Bytes on last page
  uint32_t fixup_size;       // Fixup section size
  uint32_t fixup_checksum;   // Fixup section checksum
  uint32_t loader_size;      // Loader section size
  uint32_t loader_checksum;  // Loader section checksum
  uint32_t object_table_offset;   // Object table offset
  uint32_t num_objects;      // Number of objects in module
  uint32_t object_page_map_offset; // Object page map offset
  uint32_t object_iterate_data_map_offset; // Object iterate data map offset
  uint32_t resource_table_offset; // Resource table offset
  uint32_t num_resource_entries;  // Number of resource entries
  uint32_t resident_name_table_offset; // Resident name table offset
  uint32_t entry_table_offset;    // Entry table offset
  uint32_t module_directives_offset; // Module directives offset
  uint32_t num_module_directives;    // Number of module directives
  uint32_t fixup_page_table_offset;  // Fixup page table offset
  uint32_t fixup_record_table_offset; // Fixup record table offset
  uint32_t imported_modules_table_offset; // Imported modules table offset
  uint32_t num_imported_modules;     // Number of imported module entries
  uint32_t imported_procedures_table_offset; // Imported procedures table offset
  uint32_t per_page_checksum_offset; // Per-page checksum offset
  uint32_t data_pages_offset;        // Data pages offset
  uint32_t num_preload_pages;        // Number of preload pages
  uint32_t non_resident_name_table_offset; // Non-resident name table offset
  uint32_t non_resident_name_table_length; // Non-resident name table length
  uint32_t non_resident_name_table_checksum; // Non-resident name table checksum
  uint32_t automatic_data_object; // Automatic data object
  uint32_t debug_info_offset;     // Debug info offset
  uint32_t debug_info_length;     // Debug info length
  uint32_t num_instance_preload;  // Number of instance pages in preload section
  uint32_t num_instance_demand;   // Number of instance pages in demand section
  uint32_t heap_size;             // Heap size
};

// Magic numbers
#define MZ_MAGIC          0x5A4D  // "MZ"
#define DOS4GW_LE_MAGIC   0x454C  // "LE" - Linear Executable
#define DOS4GW_LX_MAGIC   0x584C  // "LX" - Linear Executable (OS/2 2.0)

// Helper class for MZ format parsing
class MZParser {
public:
  static bool isMZ(const uint8_t *data, size_t size);
  static bool isDOS4GW(const uint8_t *data, size_t size);
  static bool parseHeader(const uint8_t *data, size_t size, MZHeader &header);
  static bool parseDOS4GWHeader(const uint8_t *data, size_t size, DOS4GWHeader &header);
  static uint32_t getEntryPoint(const MZHeader &header);
  static uint32_t getImageBase(const MZHeader &header);
  static bool getRelocations(const uint8_t *data, size_t size,
                            const MZHeader &header,
                            std::vector<MZRelocation> &relocations);
};

} // namespace MZ
} // namespace BinaryFormat
} // namespace ds2
