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
namespace SOM {

// SOM (System Object Module) - HP's native executable format
// Used on HP-UX for PA-RISC architecture
// Supports both 32-bit (SOM32) and 64-bit (SOM64)

// System ID values
enum SystemID {
  kSystemIDNone = 0,
  kSystemIDHPUX = 0x20C,     // HP-UX
  kSystemIDHPUX64 = 0x214,   // HP-UX 64-bit
  kSystemIDMPE = 0x20D       // MPE/iX
};

// CPU ID values for PA-RISC
enum CPUID {
  kCPUIDPARISC10 = 0x20B,    // PA-RISC 1.0
  kCPUIDPARISC11 = 0x210,    // PA-RISC 1.1
  kCPUIDPARISC20 = 0x214,    // PA-RISC 2.0 (64-bit)
  kCPUIDPARISC20W = 0x211    // PA-RISC 2.0 wide (32-bit mode)
};

// SOM Header (32-bit)
struct SOMHeader32 {
  uint16_t system_id;          // System ID
  uint16_t a_magic;            // Magic number
  uint32_t version_id;         // Version identifier
  uint32_t file_time;          // File timestamp
  uint32_t entry_space;        // Entry space index
  uint32_t entry_subspace;     // Entry subspace index
  uint32_t entry_offset;       // Entry point offset
  uint32_t aux_header_location;// Auxiliary header location
  uint32_t aux_header_size;    // Auxiliary header size
  uint32_t som_length;         // Length of SOM
  uint32_t presumed_dp;        // Presumed DP value
  uint32_t space_location;     // Space dictionary location
  uint32_t space_total;        // Total space entries
  uint32_t subspace_location;  // Subspace dictionary location
  uint32_t subspace_total;     // Total subspace entries
  uint32_t loader_fixup_location;   // Loader fixup location
  uint32_t loader_fixup_total; // Total loader fixup entries
  uint32_t space_strings_location;  // Space strings location
  uint32_t space_strings_size; // Space strings size
  uint32_t init_array_location;// Initialization array location
  uint32_t init_array_total;   // Initialization array total
  uint32_t compiler_location;  // Compiler location
  uint32_t compiler_total;     // Compiler total
  uint32_t symbol_location;    // Symbol table location
  uint32_t symbol_total;       // Symbol table total
  uint32_t fixup_request_location;  // Fixup request location
  uint32_t fixup_request_total;// Fixup request total
  uint32_t string_location;    // String table location
  uint32_t string_size;        // String table size
  uint32_t unloadable_sp_location;  // Unloadable space location
  uint32_t unloadable_sp_size; // Unloadable space size
  uint32_t checksum;           // Header checksum
};

// SOM Header (64-bit)
struct SOMHeader64 {
  uint16_t system_id;          // System ID
  uint16_t a_magic;            // Magic number
  uint32_t version_id;         // Version identifier
  uint64_t file_time;          // File timestamp
  uint64_t entry_space;        // Entry space index
  uint64_t entry_subspace;     // Entry subspace index
  uint64_t entry_offset;       // Entry point offset
  uint64_t aux_header_location;// Auxiliary header location
  uint64_t aux_header_size;    // Auxiliary header size
  uint64_t som_length;         // Length of SOM
  uint64_t presumed_dp;        // Presumed DP value
  uint64_t space_location;     // Space dictionary location
  uint64_t space_total;        // Total space entries
  uint64_t subspace_location;  // Subspace dictionary location
  uint64_t subspace_total;     // Total subspace entries
  uint64_t loader_fixup_location;   // Loader fixup location
  uint64_t loader_fixup_total; // Total loader fixup entries
  uint64_t space_strings_location;  // Space strings location
  uint64_t space_strings_size; // Space strings size
  uint64_t init_array_location;// Initialization array location
  uint64_t init_array_total;   // Initialization array total
  uint64_t compiler_location;  // Compiler location
  uint64_t compiler_total;     // Compiler total
  uint64_t symbol_location;    // Symbol table location
  uint64_t symbol_total;       // Symbol table total
  uint64_t fixup_request_location;  // Fixup request location
  uint64_t fixup_request_total;// Fixup request total
  uint64_t string_location;    // String table location
  uint64_t string_size;        // String table size
  uint64_t unloadable_sp_location;  // Unloadable space location
  uint64_t unloadable_sp_size; // Unloadable space size
  uint64_t checksum;           // Header checksum
};

// Magic numbers
static const uint16_t kSOMMagic32Exec = 0x0107;  // Executable
static const uint16_t kSOMMagic32Shared = 0x0108; // Shared library
static const uint16_t kSOMMagic64Exec = 0x010B;  // 64-bit executable
static const uint16_t kSOMMagic64Shared = 0x010E; // 64-bit shared library

// Space dictionary entry
struct SOMSpace {
  uint32_t name;               // Name (string table index)
  uint32_t flags;              // Space flags
  uint32_t space_number;       // Space number
  uint32_t subspace_index;     // First subspace index
  uint32_t subspace_quantity;  // Number of subspaces
  uint32_t loader_fix_index;   // Loader fixup index
  uint32_t loader_fix_quantity;// Number of loader fixups
  uint32_t init_pointer_index; // Init pointer index
  uint32_t init_pointer_quantity; // Number of init pointers
};

// Subspace dictionary entry  
struct SOMSubspace {
  uint32_t space_index;        // Space index
  uint32_t flags;              // Subspace flags
  uint32_t file_loc_init_value;// File location of initialization
  uint32_t initialization_length; // Initialization length
  uint32_t subspace_start;     // Subspace start offset
  uint32_t subspace_length;    // Subspace length
  uint32_t alignment;          // Alignment
  uint32_t name;               // Name (string table index)
  uint32_t fixup_request_index;// Fixup request index
  uint32_t fixup_request_quantity; // Number of fixup requests
};

// Subspace flags
enum SOMSubspaceFlags {
  kSubspaceCode = 0x00000001,      // Code subspace
  kSubspaceData = 0x00000002,      // Data subspace
  kSubspaceLoadable = 0x00000004,  // Loadable
  kSubspaceQuadrant = 0x000000F0,  // Quadrant bits
  kSubspaceSort = 0x00000100,      // Sort key
  kSubspaceComdat = 0x00000200,    // COMDAT
  kSubspaceCommon = 0x00000400,    // Common block
  kSubspaceDup = 0x00000800,       // Duplicate common
  kSubspaceZeroFill = 0x00001000,  // Zero-filled
};

class Loader {
public:
  Loader();
  ~Loader();

public:
  // Load SOM executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse SOM headers
  ErrorCode parseHeaders(void const *data, size_t size);

  // Get spaces and subspaces
  ErrorCode getSpaces(std::vector<SOMSpace> &spaces) const;
  ErrorCode getSubspaces(std::vector<SOMSubspace> &subspaces) const;

  // Get sections (mapped from subspaces)
  ErrorCode getSections(std::vector<Section> &sections) const;

  // Check format
  bool is64Bit() const { return _is64Bit; }
  bool isSharedLibrary() const;

private:
  union {
    SOMHeader32 header32;
    SOMHeader64 header64;
  } _header;
  
  std::vector<SOMSpace> _spaces;
  std::vector<SOMSubspace> _subspaces;
  std::vector<uint8_t> _data;
  bool _parsed;
  bool _is64Bit;

private:
  ErrorCode loadSubspace(ProcessBase *process, size_t subspaceIndex,
                        Address baseAddress, Address &subspaceAddress);
  ErrorCode processFixups(ProcessBase *process, Address baseAddress);
  
  // Read string from string table
  std::string readString(uint32_t offset) const;
};

} // namespace SOM
} // namespace Target
} // namespace ds2
