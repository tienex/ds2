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
#include <elf.h>
#include <string>
#include <vector>

namespace ds2 {
namespace Target {
namespace OS2 {

// OS/2 ELF Extensions
// OS/2 PowerPC and later x86 versions used modified ELF format
// with special sections for imports and exports

// OS/2 specific section names
#define SHN_OS2_IMPORT ".import"  // Import table section
#define SHN_OS2_EXPORT ".export"  // Export table section
#define SHN_OS2_EXCEPT ".except"  // Exception handling section

// Import table entry
struct OS2ImportEntry {
  uint32_t nameOffset;      // Offset to import name string
  uint32_t moduleOffset;    // Offset to module name string
  uint32_t ordinal;         // Import ordinal (or 0 if by name)
  uint32_t address;         // Import address (filled by loader)
};

// Export table entry
struct OS2ExportEntry {
  uint32_t nameOffset;      // Offset to export name string
  uint32_t address;         // Export address
  uint32_t ordinal;         // Export ordinal
  uint16_t flags;           // Export flags
};

// Export flags
enum OS2ExportFlags {
  kOS2ExportResident = 0x01,    // Resident export
  kOS2ExportShared = 0x02,      // Shared export
  kOS2ExportParameter = 0x04,   // Parameter export
};

class ELFExtensions {
public:
  ELFExtensions();
  ~ELFExtensions();

public:
  // Parse OS/2 ELF extensions from loaded ELF file
  ErrorCode parseExtensions(ProcessBase *process, void const *elfData,
                           size_t elfSize);

  // Get import table
  ErrorCode getImports(std::vector<OS2ImportEntry> &imports) const;

  // Get export table
  ErrorCode getExports(std::vector<OS2ExportEntry> &exports) const;

  // Resolve imports after loading
  ErrorCode resolveImports(ProcessBase *process, Address baseAddress);

  // Get import/export section offsets
  ErrorCode getImportSection(uint64_t &offset, uint64_t &size) const;
  ErrorCode getExportSection(uint64_t &offset, uint64_t &size) const;

  // Check if ELF file has OS/2 extensions
  static bool hasOS2Extensions(void const *elfData, size_t elfSize);

  // Get module name from import entry
  ErrorCode getImportModuleName(const OS2ImportEntry &entry,
                                std::string &moduleName) const;

  // Get import name
  ErrorCode getImportName(const OS2ImportEntry &entry,
                         std::string &importName) const;

  // Get export name
  ErrorCode getExportName(const OS2ExportEntry &entry,
                         std::string &exportName) const;

private:
  std::vector<OS2ImportEntry> _imports;
  std::vector<OS2ExportEntry> _exports;
  std::vector<uint8_t> _stringTable; // String table for names
  bool _parsed;

  // Section information
  uint64_t _importSectionOffset;
  uint64_t _importSectionSize;
  uint64_t _exportSectionOffset;
  uint64_t _exportSectionSize;
};

// Helper to identify OS/2 ELF files
// OS/2 ELF files have special e_ident flags or OS-specific sections
bool IsOS2ELF(void const *elfData, size_t elfSize);

// OS/2 ELF loader that extends standard ELF loader
class OS2ELFLoader {
public:
  OS2ELFLoader();
  ~OS2ELFLoader();

  // Load OS/2 ELF with extensions
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

private:
  ELFExtensions _extensions;
};

} // namespace OS2
} // namespace Target
} // namespace ds2
