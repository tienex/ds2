//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/PEF/Loader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace PEF {

static const uint32_t kPEFTag1 = 0x4A6F7921; // 'Joy!'
static const uint32_t kPEFTag2 = 0x70656666; // 'peff'
static const uint32_t kPEFPowerPC = 0x70777063; // 'pwpc'
static const uint32_t kPEFM68K = 0x6D36386B; // 'm68k'

Loader::Loader() : _parsed(false) { memset(&_header, 0, sizeof(_header)); }

Loader::~Loader() {}

ErrorCode Loader::load(ProcessBase *process, std::string const &path,
                       Address &loadAddress, Address &entryPoint) {
  // Read the PEF file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open PEF file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read PEF file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse headers
  ErrorCode error = parseHeaders(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Allocate memory for sections
  Address baseAddress = 0x10000; // Default load address for PEF
  loadAddress = baseAddress;

  // Load each section
  for (size_t i = 0; i < _sections.size(); i++) {
    const PEFSectionHeader &section = _sections[i];

    // Skip non-instantiated sections
    if (i >= _header.instSectionCount)
      continue;

    Address sectionAddress =
        baseAddress + (section.defaultAddress ? section.defaultAddress : 0);

    // Allocate memory for section
    error = process->allocateMemory(sectionAddress, section.totalLength, 0);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to allocate memory for section %zu", i);
      return error;
    }

    // Load section data based on kind
    switch (section.sectionKind) {
    case kPEFCodeSection:
    case kPEFUnpackedDataSection:
    case kPEFConstantSection:
    case kPEFExecutableDataSection: {
      // Direct copy of unpacked data
      if (section.containerLength > 0) {
        const uint8_t *sectionData = _data.data() + section.containerOffset;
        error = process->writeMemory(sectionAddress, sectionData,
                                     section.containerLength, nullptr);
        if (error != kSuccess) {
          DS2LOG(Error, "failed to write section %zu data", i);
          return error;
        }
      }
      break;
    }

    case kPEFPackedDataSection: {
      // Unpack compressed data
      std::vector<uint8_t> unpacked(section.unpackedLength);
      const uint8_t *packed = _data.data() + section.containerOffset;

      error = unpackData(packed, section.containerLength, unpacked.data(),
                        section.unpackedLength);
      if (error != kSuccess) {
        DS2LOG(Error, "failed to unpack section %zu", i);
        return error;
      }

      error = process->writeMemory(sectionAddress, unpacked.data(),
                                   section.unpackedLength, nullptr);
      if (error != kSuccess) {
        DS2LOG(Error, "failed to write unpacked section %zu data", i);
        return error;
      }
      break;
    }

    case kPEFLoaderSection:
      // Loader section is processed separately
      break;

    default:
      DS2LOG(Warning, "unknown section kind %d", section.sectionKind);
      break;
    }
  }

  // Process loader section for relocations and entry point
  PEFLoaderInfoHeader loaderHeader;
  error = readLoaderSection(loaderHeader);
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to read loader section");
  } else {
    // Calculate entry point
    if (loaderHeader.mainSection >= 0 &&
        loaderHeader.mainSection < (int32_t)_sections.size()) {
      const PEFSectionHeader &mainSection =
          _sections[loaderHeader.mainSection];
      entryPoint = baseAddress + mainSection.defaultAddress +
                   loaderHeader.mainOffset;
    } else {
      // Use first code section as entry point
      for (size_t i = 0; i < _sections.size(); i++) {
        if (_sections[i].sectionKind == kPEFCodeSection) {
          entryPoint = baseAddress + _sections[i].defaultAddress;
          break;
        }
      }
    }

    // Process relocations
    error = processRelocations(process, baseAddress);
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to process relocations");
    }
  }

  return kSuccess;
}

ErrorCode Loader::parseHeaders(void const *data, size_t size) {
  if (size < sizeof(PEFContainerHeader)) {
    DS2LOG(Error, "file too small for PEF header");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  // Read container header
  memcpy(&_header, ptr, sizeof(PEFContainerHeader));
  ptr += sizeof(PEFContainerHeader);

  // Verify magic numbers
  if (_header.tag1 != kPEFTag1 || _header.tag2 != kPEFTag2) {
    DS2LOG(Error, "invalid PEF magic numbers: 0x%08x 0x%08x", _header.tag1,
           _header.tag2);
    return kErrorInvalidArgument;
  }

  // Verify architecture
  if (_header.architecture != kPEFPowerPC &&
      _header.architecture != kPEFM68K) {
    DS2LOG(Error, "unsupported PEF architecture: 0x%08x",
           _header.architecture);
    return kErrorUnsupported;
  }

  DS2LOG(Debug, "PEF file: architecture=0x%08x, sections=%d",
         _header.architecture, _header.sectionCount);

  // Read section headers
  _sections.clear();
  for (uint16_t i = 0; i < _header.sectionCount; i++) {
    if (ptr + sizeof(PEFSectionHeader) >
        static_cast<const uint8_t *>(data) + size) {
      DS2LOG(Error, "file too small for section header %d", i);
      return kErrorInvalidArgument;
    }

    PEFSectionHeader section;
    memcpy(&section, ptr, sizeof(PEFSectionHeader));
    ptr += sizeof(PEFSectionHeader);

    _sections.push_back(section);

    DS2LOG(Debug,
           "Section %d: kind=%d, offset=0x%08x, length=0x%08x, "
           "unpacked=0x%08x",
           i, section.sectionKind, section.containerOffset,
           section.containerLength, section.unpackedLength);
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode Loader::readLoaderSection(PEFLoaderInfoHeader &loaderHeader) {
  if (!_parsed)
    return kErrorInvalidArgument;

  // Find loader section
  for (size_t i = 0; i < _sections.size(); i++) {
    if (_sections[i].sectionKind == kPEFLoaderSection) {
      if (_sections[i].containerOffset + sizeof(PEFLoaderInfoHeader) >
          _data.size()) {
        return kErrorInvalidArgument;
      }

      const uint8_t *loaderData = _data.data() + _sections[i].containerOffset;
      memcpy(&loaderHeader, loaderData, sizeof(PEFLoaderInfoHeader));
      return kSuccess;
    }
  }

  return kErrorNotFound;
}

ErrorCode Loader::processRelocations(ProcessBase *process,
                                     Address baseAddress) {
  // PEF relocations are complex and would require extensive implementation
  // This is a placeholder for the relocation processing logic
  DS2LOG(Debug, "processing PEF relocations at base 0x%llx",
         (unsigned long long)baseAddress.value());
  return kSuccess;
}

ErrorCode Loader::unpackData(void const *packed, size_t packedSize,
                             void *unpacked, size_t unpackedSize) {
  // PEF uses a simple run-length encoding scheme
  // This is a simplified implementation
  const uint8_t *src = static_cast<const uint8_t *>(packed);
  uint8_t *dst = static_cast<uint8_t *>(unpacked);
  const uint8_t *srcEnd = src + packedSize;
  uint8_t *dstEnd = dst + unpackedSize;

  while (src < srcEnd && dst < dstEnd) {
    uint8_t opcode = *src++;

    if (opcode == 0) {
      // Zero fill
      if (src >= srcEnd)
        break;
      uint8_t count = *src++;
      if (dst + count > dstEnd)
        return kErrorInvalidArgument;
      memset(dst, 0, count);
      dst += count;
    } else if (opcode <= 0x7F) {
      // Copy literal bytes
      if (src + opcode > srcEnd || dst + opcode > dstEnd)
        return kErrorInvalidArgument;
      memcpy(dst, src, opcode);
      src += opcode;
      dst += opcode;
    } else {
      // Repeat previous pattern
      uint8_t count = opcode & 0x7F;
      if (src >= srcEnd)
        break;
      uint8_t offset = *src++;
      if (dst - offset < static_cast<uint8_t *>(unpacked) ||
          dst + count > dstEnd)
        return kErrorInvalidArgument;

      for (uint8_t i = 0; i < count; i++) {
        *dst = *(dst - offset);
        dst++;
      }
    }
  }

  return kSuccess;
}

ErrorCode Loader::getSections(std::vector<Section> &sections) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  sections.clear();
  for (const auto &pefSection : _sections) {
    Section section;
    section.start = pefSection.defaultAddress;
    section.length = pefSection.totalLength;
    sections.push_back(section);
  }

  return kSuccess;
}

ErrorCode Loader::getImportedLibraries(std::vector<std::string> &libraries) const {
  // Would parse loader section for imported library names
  libraries.clear();
  return kSuccess;
}

ErrorCode Loader::getExportedSymbols(std::vector<std::string> &symbols) const {
  // Would parse loader section for exported symbol names
  symbols.clear();
  return kSuccess;
}

} // namespace PEF
} // namespace Target
} // namespace ds2
