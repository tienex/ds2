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
namespace PEF {

// PEF (Preferred Executable Format) - used on Mac OS Classic and BeOS PowerPC
// File format created by Apple for PowerPC executables

// PEF Container Header
struct PEFContainerHeader {
  uint32_t tag1;              // 'Joy!' (0x4A6F7921)
  uint32_t tag2;              // 'peff' (0x70656666)
  uint32_t architecture;      // 'pwpc' (PowerPC) or 'm68k'
  uint32_t formatVersion;     // Version number
  uint32_t dateTimeStamp;     // Creation timestamp
  uint32_t oldDefVersion;     // Old definition version
  uint32_t oldImpVersion;     // Old implementation version
  uint32_t currentVersion;    // Current version
  uint16_t sectionCount;      // Number of sections
  uint16_t instSectionCount;  // Number of instantiated sections
  uint32_t reservedA;         // Reserved
};

// PEF Section Header
struct PEFSectionHeader {
  int32_t nameOffset;         // Offset to section name (or -1)
  uint32_t defaultAddress;    // Default load address
  uint32_t totalLength;       // Total section length
  uint32_t unpackedLength;    // Unpacked data length
  uint32_t containerLength;   // Packed data length in container
  uint32_t containerOffset;   // Offset to section data
  uint8_t sectionKind;        // Section type
  uint8_t shareKind;          // Sharing type
  uint8_t alignment;          // Alignment (power of 2)
  uint8_t reserved;           // Reserved
};

// Section kinds
enum PEFSectionKind {
  kPEFCodeSection = 0,        // Executable code
  kPEFUnpackedDataSection = 1, // Unpacked data
  kPEFPackedDataSection = 2,  // Packed data
  kPEFConstantSection = 3,    // Read-only data
  kPEFLoaderSection = 4,      // Loader information
  kPEFDebugSection = 5,       // Debug symbols
  kPEFExecutableDataSection = 6, // Executable data
  kPEFExceptionSection = 7,   // Exception handling
  kPEFTracebackSection = 8    // Traceback tables
};

// PEF Loader Header (in loader section)
struct PEFLoaderInfoHeader {
  int32_t mainSection;        // Main symbol section
  uint32_t mainOffset;        // Main symbol offset
  int32_t initSection;        // Init routine section
  uint32_t initOffset;        // Init routine offset
  int32_t termSection;        // Term routine section
  uint32_t termOffset;        // Term routine offset
  uint32_t importedLibraryCount; // Number of imported libraries
  uint32_t totalImportedSymbolCount; // Total imported symbols
  uint32_t relocSectionCount; // Number of relocation sections
  uint32_t relocInstrOffset;  // Offset to relocation instructions
  uint32_t loaderStringsOffset; // Offset to loader strings
  uint32_t exportHashOffset;  // Offset to export hash table
  uint32_t exportHashTablePower; // Export hash table size (2^n)
  uint32_t exportedSymbolCount; // Number of exported symbols
};

class Loader {
public:
  Loader();
  ~Loader();

public:
  // Load PEF executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse PEF headers
  ErrorCode parseHeaders(void const *data, size_t size);

  // Get section information
  ErrorCode getSections(std::vector<Section> &sections) const;

  // Get imported libraries
  ErrorCode getImportedLibraries(std::vector<std::string> &libraries) const;

  // Get exported symbols
  ErrorCode getExportedSymbols(std::vector<std::string> &symbols) const;

  // Unpack packed data section
  static ErrorCode unpackData(void const *packed, size_t packedSize,
                              void *unpacked, size_t unpackedSize);

private:
  PEFContainerHeader _header;
  std::vector<PEFSectionHeader> _sections;
  std::vector<uint8_t> _data;
  bool _parsed;

private:
  ErrorCode readLoaderSection(PEFLoaderInfoHeader &loaderHeader);
  ErrorCode processRelocations(ProcessBase *process, Address baseAddress);
};

} // namespace PEF
} // namespace Target
} // namespace ds2
