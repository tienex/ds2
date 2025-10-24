//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/OpenVMS/ImageLoader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace OpenVMS {

ImageLoader::ImageLoader()
    : _parsed(false), _isVAX(false), _isAlpha(false), _isIA64(false),
      _isx64(false) {
  memset(&_header, 0, sizeof(_header));
}

ImageLoader::~ImageLoader() {}

ErrorCode ImageLoader::load(ProcessBase *process, std::string const &path,
                            Address &loadAddress, Address &entryPoint) {
  // Read the OpenVMS image file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open VMS image file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read VMS image file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse headers
  ErrorCode error = parseHeaders(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address based on architecture
  Address baseAddress;
  if (_isVAX) {
    baseAddress = 0x00010000; // VAX typical load address
  } else if (_isAlpha) {
    baseAddress = 0x0000000120000000ULL; // Alpha typical load address
  } else if (_isIA64 || _isx64) {
    baseAddress = 0x0000000080000000ULL; // IA-64/x64 typical load address
  } else {
    baseAddress = 0x00010000; // Default
  }

  loadAddress = baseAddress;

  // Load all sections
  for (size_t i = 0; i < _sections.size(); i++) {
    Address sectionAddr;
    error = loadSection(process, i, baseAddress, sectionAddr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to load section %zu", i);
      return error;
    }
  }

  // Process fixups
  error = processFixups(process, baseAddress);
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to process fixups");
  }

  // Entry point from image activator virtual address
  if (_header.iafva != 0) {
    entryPoint = baseAddress + _header.iafva;
  } else {
    entryPoint = baseAddress;
  }

  DS2LOG(Debug, "VMS image loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode ImageLoader::parseHeaders(void const *data, size_t size) {
  if (size < sizeof(ImageHeader)) {
    DS2LOG(Error, "file too small for VMS image header");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  // Read image header (little-endian on all VMS platforms)
  memcpy(&_header, ptr, sizeof(ImageHeader));

  // Detect architecture from header flags and structure
  // This is a simplified detection - real VMS images have more complex markers
  if (_header.version >= 1 && _header.version <= 3) {
    _isVAX = true;
    DS2LOG(Debug, "Detected VAX image");
  } else if (_header.version >= 4 && _header.version <= 5) {
    _isAlpha = true;
    DS2LOG(Debug, "Detected Alpha image");
  } else if (_header.version == 6) {
    _isIA64 = true;
    DS2LOG(Debug, "Detected IA-64 image");
  } else if (_header.version == 7) {
    _isx64 = true;
    DS2LOG(Debug, "Detected x86-64 image");
  }

  DS2LOG(Debug,
         "VMS Image: version=%u, iafva=0x%x, symvva=0x%x, symvblks=%u",
         _header.version, _header.iafva, _header.symvva, _header.symvblks);

  // Parse image section descriptors (ISDs)
  // ISDs typically start after the header
  ptr += _header.size;

  while (ptr + sizeof(ImageSectionDescriptor) <= data + size) {
    ImageSectionDescriptor isd;
    memcpy(&isd, ptr, sizeof(ImageSectionDescriptor));

    // Check for end of ISDs (size = 0)
    if (isd.size == 0)
      break;

    _sections.push_back(isd);

    DS2LOG(Debug, "ISD %zu: type=%u, flags=0x%x, vbn=%u, pagcnt=%u",
           _sections.size() - 1, isd.type, isd.flags, isd.vbn, isd.pagcnt);

    ptr += isd.size;
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode ImageLoader::loadSection(ProcessBase *process, size_t sectionIndex,
                                   Address baseAddress,
                                   Address &sectionAddress) {
  if (sectionIndex >= _sections.size())
    return kErrorInvalidArgument;

  const ImageSectionDescriptor &isd = _sections[sectionIndex];

  // Skip non-loadable sections (fixup, debug, etc.)
  if (isd.type == kISDTypeFixup || isd.type == kISDTypeDebug ||
      isd.type == kISDTypeGlobalSymbol) {
    return kSuccess;
  }

  // Calculate section address
  if (_isVAX) {
    sectionAddress = baseAddress + isd.vaddr;
  } else {
    sectionAddress = baseAddress + isd.vaddr64;
  }

  // Calculate section size (page count * 512 bytes per VMS page)
  uint32_t sectionSize = isd.pagcnt * 512;

  // Determine protection flags
  uint32_t protection = 0x01; // Read
  if (isd.flags & kISDFlagWritable)
    protection |= 0x02; // Write
  if (isd.flags & kISDFlagExecutable)
    protection |= 0x04; // Execute

  // Allocate memory
  ErrorCode error = process->allocateMemory(sectionAddress, sectionSize,
                                           protection);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to allocate memory for section %zu", sectionIndex);
    return error;
  }

  // Load section data
  if (isd.type == kISDTypeDemandZero) {
    // Zero-fill demand-zero sections
    std::vector<uint8_t> zeros(sectionSize, 0);
    error = process->writeMemory(sectionAddress, zeros.data(), sectionSize,
                                 nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to zero-fill section %zu", sectionIndex);
      return error;
    }
  } else {
    // Load from file (VBN points to virtual block number)
    uint32_t fileOffset = (isd.vbn - 1) * 512; // VBN is 1-based

    if (fileOffset + sectionSize > _data.size()) {
      DS2LOG(Warning, "section %zu data extends beyond file", sectionIndex);
      sectionSize = _data.size() - fileOffset;
    }

    if (fileOffset < _data.size()) {
      error = process->writeMemory(sectionAddress, _data.data() + fileOffset,
                                   sectionSize, nullptr);
      if (error != kSuccess) {
        DS2LOG(Error, "failed to write section %zu data", sectionIndex);
        return error;
      }
    }
  }

  return kSuccess;
}

ErrorCode ImageLoader::processFixups(ProcessBase *process,
                                     Address baseAddress) {
  // Find fixup sections
  for (size_t i = 0; i < _sections.size(); i++) {
    const ImageSectionDescriptor &isd = _sections[i];

    if (isd.type != kISDTypeFixup)
      continue;

    // Read fixup data
    uint32_t fileOffset = (isd.vbn - 1) * 512;
    uint32_t fixupSize = isd.pagcnt * 512;

    if (fileOffset + fixupSize > _data.size()) {
      DS2LOG(Warning, "fixup section %zu data extends beyond file", i);
      continue;
    }

    const uint8_t *fixupData = _data.data() + fileOffset;
    const uint8_t *fixupEnd = fixupData + fixupSize;

    // Process fixup records
    // This is a simplified implementation - real VMS fixups are complex
    while (fixupData + 8 <= fixupEnd) {
      uint32_t fixupType = *reinterpret_cast<const uint32_t *>(fixupData);
      fixupData += 4;

      uint32_t fixupOffset = *reinterpret_cast<const uint32_t *>(fixupData);
      fixupData += 4;

      if (fixupType == 0)
        break; // End of fixups

      Address fixupAddr = baseAddress + fixupOffset;

      // Read current value
      uint64_t value;
      size_t valueSize = (_isVAX || _isIA64) ? 8 : 4;

      ErrorCode error = process->readMemory(fixupAddr, &value, valueSize,
                                           nullptr);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed to read fixup at offset 0x%x", fixupOffset);
        continue;
      }

      // Apply fixup based on type
      switch (fixupType) {
      case kFixupAbsolute:
        value += baseAddress.value();
        break;
      case kFixupSelfRel:
        value += fixupAddr.value();
        break;
      default:
        DS2LOG(Warning, "unknown fixup type: %u", fixupType);
        continue;
      }

      error = process->writeMemory(fixupAddr, &value, valueSize, nullptr);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed to write fixup at offset 0x%x", fixupOffset);
        continue;
      }
    }
  }

  return kSuccess;
}

ErrorCode ImageLoader::getSections(
    std::vector<ImageSectionDescriptor> &sections) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  sections = _sections;
  return kSuccess;
}

std::string ImageLoader::readString(uint32_t offset) const {
  if (offset >= _data.size())
    return "";

  const char *str = reinterpret_cast<const char *>(_data.data() + offset);
  size_t maxLen = _data.size() - offset;

  return std::string(str, strnlen(str, maxLen));
}

} // namespace OpenVMS
} // namespace Target
} // namespace ds2
