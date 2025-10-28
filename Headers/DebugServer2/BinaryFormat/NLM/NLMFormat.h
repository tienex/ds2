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
#include <string>

namespace ds2 {
namespace BinaryFormat {
namespace NLM {

// NLM (NetWare Loadable Module) Executable Format
// Used by Novell NetWare server modules
// Supports NetWare 3.x, 4.x, 5.x, 6.x

// NLM Header (Version 0/1 - NetWare 3.x, 4.x)
struct NLMHeader {
  char signature[24];         // NLM signature string
  uint32_t version;           // NLM version
  char module_name[14];       // Module name
  uint32_t code_image_offset; // Code image offset
  uint32_t code_image_size;   // Code image size
  uint32_t data_image_offset; // Data image offset
  uint32_t data_image_size;   // Data image size
  uint32_t uninit_data_size;  // Uninitialized data size
  uint32_t custom_data_offset; // Custom data offset
  uint32_t custom_data_size;  // Custom data size
  uint32_t module_depend_offset; // Module dependencies offset
  uint32_t module_depend_size;   // Module dependencies size
  uint32_t reloc_fixup_offset;   // Relocation fixup offset
  uint32_t reloc_fixup_size;     // Relocation fixup size
  uint32_t external_ref_offset;  // External references offset
  uint32_t external_ref_size;    // External references size
  uint32_t publics_offset;    // Public symbols offset
  uint32_t publics_size;      // Public symbols size
  uint32_t debug_info_offset; // Debug info offset
  uint32_t debug_info_size;   // Debug info size
  uint32_t code_start_offset; // Code start offset (entry point)
  uint32_t exit_procedure_offset; // Exit procedure offset
  uint32_t check_unload_offset;   // Check unload procedure offset
  uint32_t module_type;       // Module type
  uint32_t flags;             // Module flags
};

// Extended NLM Header (Version 2 - NetWare 5.x, 6.x)
struct NLMHeaderV2 {
  char signature[64];         // Extended signature
  uint32_t nlm_version;       // NLM format version (2)
  char module_name[128];      // Module name (longer)
  uint32_t flags;             // Module flags
  uint32_t module_type;       // Module type

  // Code segments
  uint32_t code_offset;       // Code offset
  uint32_t code_size;         // Code size

  // Data segments
  uint32_t data_offset;       // Data offset
  uint32_t data_size;         // Data size
  uint32_t bss_size;          // BSS size

  // Entry points
  uint32_t start_address;     // Start address (entry point)
  uint32_t exit_address;      // Exit procedure address
  uint32_t check_unload_address; // Check unload address

  // Symbol tables
  uint32_t publics_offset;    // Public symbols offset
  uint32_t publics_count;     // Number of public symbols
  uint32_t externals_offset;  // External references offset
  uint32_t externals_count;   // Number of external references

  // Relocations
  uint32_t fixups_offset;     // Fixup/relocation offset
  uint32_t fixups_count;      // Number of fixups

  // Dependencies
  uint32_t imports_offset;    // Imported modules offset
  uint32_t imports_count;     // Number of imports

  // Debug information
  uint32_t debug_offset;      // Debug info offset
  uint32_t debug_size;        // Debug info size

  // Additional metadata
  uint32_t copyright_offset;  // Copyright string offset
  uint32_t copyright_size;    // Copyright string size
  uint32_t description_offset; // Description offset
  uint32_t description_size;  // Description size

  // Thread information
  uint32_t stack_size;        // Default stack size
  uint32_t thread_name_offset; // Thread name offset
  uint32_t thread_name_size;  // Thread name size

  // Screen information
  uint32_t screen_name_offset; // Screen name offset
  uint32_t screen_name_size;  // Screen name size

  // Resource information
  uint32_t rp_count;          // Number of resource tags
  uint32_t rp_offset;         // Resource tags offset

  // Custom data
  uint32_t custom_data_offset; // Custom data offset
  uint32_t custom_data_size;  // Custom data size

  // Extended attributes
  uint32_t messages_offset;   // Messages offset
  uint32_t messages_size;     // Messages size
  uint32_t help_offset;       // Help data offset
  uint32_t help_size;         // Help data size

  // Security
  uint32_t signature_offset;  // Digital signature offset
  uint32_t signature_size;    // Digital signature size
};

// NLM signature strings
#define NLM_SIGNATURE_V0  "NetWare Loadable Module"
#define NLM_SIGNATURE_V1  "NetWare 386 Loadable Module"
#define NLM_SIGNATURE_V2  "NetWare Loadable Module V2"

// Module types
enum NLMModuleType {
  NLM_TYPE_GENERIC = 0,       // Generic NLM
  NLM_TYPE_LAN_DRIVER = 1,    // LAN driver
  NLM_TYPE_DISK_DRIVER = 2,   // Disk driver
  NLM_TYPE_NAME_SPACE = 3,    // Name space module
  NLM_TYPE_UTILITY = 4,       // Utility
  NLM_TYPE_MSL = 5,           // Mirrored Server Link
  NLM_TYPE_SERVER_APP = 6,    // Server application
  NLM_TYPE_HIDDEN = 7,        // Hidden module
  NLM_TYPE_OS_MODULE = 8,     // Operating system module
  NLM_TYPE_HAM = 9,           // Host Adapter Module
  NLM_TYPE_CDM = 10,          // Custom Device Module
  NLM_TYPE_FILE_SYSTEM = 11,  // File system
  NLM_TYPE_MONITOR = 12,      // Monitor NLM
  NLM_TYPE_FABRIC_DRIVER = 13, // Fabric driver
};

// Module flags
enum NLMFlags {
  NLM_FLAG_REENTRANT = 0x00000001,      // Module is reentrant
  NLM_FLAG_MULTIPROCESSOR = 0x00000002, // MP safe
  NLM_FLAG_SYNCHRONIZE = 0x00000004,    // Synchronize start
  NLM_FLAG_PSEUDOPREEMPTION = 0x00000008, // Pseudo-preemption
  NLM_FLAG_OS_DOMAIN = 0x00000010,      // OS domain module
  NLM_FLAG_LIBRARY = 0x00000020,        // Library NLM
  NLM_FLAG_AUTO_UNLOAD = 0x00000040,    // Auto-unload when not in use
  NLM_FLAG_RING3 = 0x00000080,          // Ring 3 module
  NLM_FLAG_NO_SCREENS = 0x00000100,     // Don't create screens
  NLM_FLAG_PROTECTED = 0x00000200,      // Protected NLM
};

// Fixup/Relocation entry
struct NLMFixup {
  uint8_t type;               // Fixup type
  uint8_t flags;              // Fixup flags
  uint16_t reserved;          // Reserved
  uint32_t offset;            // Offset needing fixup
  uint32_t target;            // Target address/symbol
};

// Fixup types
enum NLMFixupType {
  NLM_FIXUP_LOW_BYTE = 0,         // Low byte
  NLM_FIXUP_OFFSET_16 = 1,        // 16-bit offset
  NLM_FIXUP_BASE = 2,             // Base (segment)
  NLM_FIXUP_POINTER_16 = 3,       // 16-bit pointer (base:offset)
  NLM_FIXUP_OFFSET_32 = 4,        // 32-bit offset
  NLM_FIXUP_POINTER_32 = 5,       // 32-bit pointer
  NLM_FIXUP_SELF_RELATIVE = 6,    // Self-relative offset
};

// Public symbol entry
struct NLMPublicSymbol {
  char name[256];             // Symbol name
  uint32_t offset;            // Offset in code/data
  uint8_t type;               // Symbol type (code/data)
  uint8_t reserved[3];        // Reserved
};

// External reference entry
struct NLMExternalRef {
  char name[256];             // Symbol name
  uint32_t num_references;    // Number of references
  uint32_t *ref_offsets;      // Array of reference offsets
};

// Module dependency entry
struct NLMModuleDependency {
  char module_name[128];      // Dependent module name
  uint32_t min_version;       // Minimum required version
  uint32_t max_version;       // Maximum compatible version
};

// Debug information structure (CodeView format or NetWare specific)
struct NLMDebugInfo {
  uint32_t format;            // Debug format (0=none, 1=CodeView, 2=NetWare)
  uint32_t version;           // Debug info version
  uint32_t symbols_offset;    // Symbols offset
  uint32_t symbols_size;      // Symbols size
  uint32_t types_offset;      // Types offset
  uint32_t types_size;        // Types size
  uint32_t lines_offset;      // Line numbers offset
  uint32_t lines_size;        // Line numbers size
};

// Resource tag
struct NLMResourceTag {
  char tag_signature[16];     // Resource tag signature
  uint32_t data_offset;       // Resource data offset
  uint32_t data_size;         // Resource data size
};

// Common resource tags
#define NLM_RESOURCE_COPYRIGHT    "Copyright"
#define NLM_RESOURCE_DESCRIPTION  "Description"
#define NLM_RESOURCE_VERSION      "Version"
#define NLM_RESOURCE_MESSAGES     "Messages"
#define NLM_RESOURCE_HELP         "Help"

// Helper class for NLM format parsing
class NLMParser {
public:
  static bool isNLM(const uint8_t *data, size_t size);
  static uint32_t getNLMVersion(const uint8_t *data, size_t size);
  static bool parseHeader(const uint8_t *data, size_t size, NLMHeader &header);
  static bool parseHeaderV2(const uint8_t *data, size_t size, NLMHeaderV2 &header);

  static bool parsePublicSymbols(const uint8_t *data, size_t size,
                                 const NLMHeader &header,
                                 std::vector<NLMPublicSymbol> &symbols);
  static bool parseExternalRefs(const uint8_t *data, size_t size,
                               const NLMHeader &header,
                               std::vector<NLMExternalRef> &externals);
  static bool parseFixups(const uint8_t *data, size_t size,
                         const NLMHeader &header,
                         std::vector<NLMFixup> &fixups);
  static bool parseDependencies(const uint8_t *data, size_t size,
                               const NLMHeader &header,
                               std::vector<NLMModuleDependency> &deps);
  static bool parseDebugInfo(const uint8_t *data, size_t size,
                            const NLMHeader &header,
                            NLMDebugInfo &debug);

  static uint32_t getEntryPoint(const NLMHeader &header);
  static std::string getModuleName(const NLMHeader &header);
  static std::string getModuleTypeName(uint32_t type);
  static bool hasDebugInfo(const NLMHeader &header);
  static bool isReentrant(const NLMHeader &header);
  static bool isMultiprocessorSafe(const NLMHeader &header);
};

} // namespace NLM
} // namespace BinaryFormat
} // namespace ds2
