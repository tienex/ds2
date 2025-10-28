//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/OS2/ELFExtensions.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <elf.h>

namespace ds2 {
namespace Target {
namespace OS2 {

ELFExtensions::ELFExtensions()
    : _parsed(false), _importSectionOffset(0), _importSectionSize(0),
      _exportSectionOffset(0), _exportSectionSize(0) {}

ELFExtensions::~ELFExtensions() {}

bool ELFExtensions::hasOS2Extensions(void const *elfData, size_t elfSize) {
  if (elfSize < sizeof(Elf32_Ehdr))
    return false;

  const Elf32_Ehdr *ehdr = static_cast<const Elf32_Ehdr *>(elfData);

  // Check ELF magic
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    return false;

  // OS/2 uses ELFOSABI_NONE or custom OSABI value
  // Check for .import or .export sections
  if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0)
    return false;

  // Read section headers
  const uint8_t *data = static_cast<const uint8_t *>(elfData);
  const Elf32_Shdr *shdr =
      reinterpret_cast<const Elf32_Shdr *>(data + ehdr->e_shoff);

  // Get section string table
  if (ehdr->e_shstrndx >= ehdr->e_shnum)
    return false;

  const Elf32_Shdr *shstrtab = &shdr[ehdr->e_shstrndx];
  if (shstrtab->sh_offset + shstrtab->sh_size > elfSize)
    return false;

  const char *shstrtab_data =
      reinterpret_cast<const char *>(data + shstrtab->sh_offset);

  // Look for .import or .export sections
  for (int i = 0; i < ehdr->e_shnum; i++) {
    if (shdr[i].sh_name >= shstrtab->sh_size)
      continue;

    const char *name = shstrtab_data + shdr[i].sh_name;

    if (strcmp(name, SHN_OS2_IMPORT) == 0 ||
        strcmp(name, SHN_OS2_EXPORT) == 0) {
      return true;
    }
  }

  return false;
}

ErrorCode ELFExtensions::parseExtensions(ProcessBase *process,
                                        void const *elfData, size_t elfSize) {
  if (elfSize < sizeof(Elf32_Ehdr)) {
    DS2LOG(Error, "ELF file too small");
    return kErrorInvalidArgument;
  }

  const Elf32_Ehdr *ehdr = static_cast<const Elf32_Ehdr *>(elfData);
  const uint8_t *data = static_cast<const uint8_t *>(elfData);

  // Check ELF magic
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    DS2LOG(Error, "invalid ELF magic");
    return kErrorInvalidArgument;
  }

  // Read section headers
  const Elf32_Shdr *shdr =
      reinterpret_cast<const Elf32_Shdr *>(data + ehdr->e_shoff);
  const Elf32_Shdr *shstrtab = &shdr[ehdr->e_shstrndx];
  const char *shstrtab_data =
      reinterpret_cast<const char *>(data + shstrtab->sh_offset);

  // Find .import and .export sections
  const Elf32_Shdr *importSection = nullptr;
  const Elf32_Shdr *exportSection = nullptr;

  for (int i = 0; i < ehdr->e_shnum; i++) {
    if (shdr[i].sh_name >= shstrtab->sh_size)
      continue;

    const char *name = shstrtab_data + shdr[i].sh_name;

    if (strcmp(name, SHN_OS2_IMPORT) == 0) {
      importSection = &shdr[i];
      _importSectionOffset = shdr[i].sh_offset;
      _importSectionSize = shdr[i].sh_size;
      DS2LOG(Debug, "found .import section at offset 0x%llx, size 0x%llx",
             (unsigned long long)_importSectionOffset,
             (unsigned long long)_importSectionSize);
    } else if (strcmp(name, SHN_OS2_EXPORT) == 0) {
      exportSection = &shdr[i];
      _exportSectionOffset = shdr[i].sh_offset;
      _exportSectionSize = shdr[i].sh_size;
      DS2LOG(Debug, "found .export section at offset 0x%llx, size 0x%llx",
             (unsigned long long)_exportSectionOffset,
             (unsigned long long)_exportSectionSize);
    }
  }

  // Parse import section
  if (importSection) {
    const uint8_t *importData = data + importSection->sh_offset;
    size_t numImports = importSection->sh_size / sizeof(OS2ImportEntry);

    _imports.clear();
    for (size_t i = 0; i < numImports; i++) {
      OS2ImportEntry entry;
      memcpy(&entry, importData + i * sizeof(OS2ImportEntry),
             sizeof(OS2ImportEntry));
      _imports.push_back(entry);
    }

    DS2LOG(Debug, "parsed %zu import entries", _imports.size());
  }

  // Parse export section
  if (exportSection) {
    const uint8_t *exportData = data + exportSection->sh_offset;
    size_t numExports = exportSection->sh_size / sizeof(OS2ExportEntry);

    _exports.clear();
    for (size_t i = 0; i < numExports; i++) {
      OS2ExportEntry entry;
      memcpy(&entry, exportData + i * sizeof(OS2ExportEntry),
             sizeof(OS2ExportEntry));
      _exports.push_back(entry);
    }

    DS2LOG(Debug, "parsed %zu export entries", _exports.size());
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode ELFExtensions::getImports(std::vector<OS2ImportEntry> &imports) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  imports = _imports;
  return kSuccess;
}

ErrorCode ELFExtensions::getExports(std::vector<OS2ExportEntry> &exports) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  exports = _exports;
  return kSuccess;
}

ErrorCode ELFExtensions::resolveImports(ProcessBase *process,
                                       Address baseAddress) {
  // Import resolution would involve:
  // 1. Loading referenced DLLs
  // 2. Resolving import addresses
  // 3. Patching import address table
  DS2LOG(Debug, "resolving %zu imports", _imports.size());
  return kSuccess;
}

ErrorCode ELFExtensions::getImportSection(uint64_t &offset,
                                          uint64_t &size) const {
  if (!_parsed || _importSectionSize == 0)
    return kErrorNotFound;

  offset = _importSectionOffset;
  size = _importSectionSize;
  return kSuccess;
}

ErrorCode ELFExtensions::getExportSection(uint64_t &offset,
                                          uint64_t &size) const {
  if (!_parsed || _exportSectionSize == 0)
    return kErrorNotFound;

  offset = _exportSectionOffset;
  size = _exportSectionSize;
  return kSuccess;
}

bool IsOS2ELF(void const *elfData, size_t elfSize) {
  return ELFExtensions::hasOS2Extensions(elfData, elfSize);
}

OS2ELFLoader::OS2ELFLoader() {}

OS2ELFLoader::~OS2ELFLoader() {}

ErrorCode OS2ELFLoader::load(ProcessBase *process, std::string const &path,
                              Address &loadAddress, Address &entryPoint) {
  // Would use standard ELF loader with OS/2 extensions
  DS2LOG(Debug, "loading OS/2 ELF: %s", path.c_str());

  // Parse extensions
  // Load ELF segments
  // Resolve imports
  // Set entry point

  return kSuccess;
}

} // namespace OS2
} // namespace Target
} // namespace ds2
