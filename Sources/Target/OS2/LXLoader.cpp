//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/OS2/LXLoader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace OS2 {

static const uint16_t kLXMagic = 0x584C; // 'LX'
static const uint16_t kLEMagic = 0x454C; // 'LE'
static const uint16_t kMZMagic = 0x5A4D; // 'MZ' (DOS stub)

LXLoader::LXLoader() : _parsed(false), _isLE(false) {
  memset(&_header, 0, sizeof(_header));
}

LXLoader::~LXLoader() {}

ErrorCode LXLoader::load(ProcessBase *process, std::string const &path,
                         Address &loadAddress, Address &entryPoint) {
  // Read the LX/LE file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open LX/LE file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read LX/LE file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse headers
  ErrorCode error = parseHeaders(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address
  Address baseAddress = 0x10000; // Default load address for OS/2 apps
  loadAddress = baseAddress;

  // Load all objects
  for (size_t i = 0; i < _objects.size(); i++) {
    Address objectAddress;
    error = loadObject(process, i, baseAddress, objectAddress);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to load object %zu", i);
      return error;
    }
  }

  // Process fixups (relocations)
  error = processFixups(process, baseAddress);
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to process fixups");
  }

  // Calculate entry point
  if (_header.eipObject > 0 && _header.eipObject <= _objects.size()) {
    const LXObject &eipObj = _objects[_header.eipObject - 1];
    entryPoint = baseAddress + eipObj.relocBaseAddr + _header.eip;
  } else {
    // Fallback to first executable object
    for (size_t i = 0; i < _objects.size(); i++) {
      if (_objects[i].objectFlags & kLXObjectExecutable) {
        entryPoint = baseAddress + _objects[i].relocBaseAddr;
        break;
      }
    }
  }

  DS2LOG(Debug, "LX/LE loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode LXLoader::parseHeaders(void const *data, size_t size) {
  if (size < sizeof(uint16_t) * 2) {
    DS2LOG(Error, "file too small for any executable");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  // Check for MZ (DOS) stub
  uint16_t mzMagic;
  memcpy(&mzMagic, ptr, sizeof(mzMagic));

  uint32_t lxOffset = 0;
  if (mzMagic == kMZMagic) {
    // DOS stub present, read offset to LX header at 0x3C
    if (size < 0x40) {
      DS2LOG(Error, "DOS stub too small");
      return kErrorInvalidArgument;
    }
    memcpy(&lxOffset, ptr + 0x3C, sizeof(lxOffset));
  }

  // Verify we have enough space for LX header
  if (lxOffset + sizeof(LXHeader) > size) {
    DS2LOG(Error, "file too small for LX header");
    return kErrorInvalidArgument;
  }

  ptr += lxOffset;

  // Read LX/LE header
  memcpy(&_header, ptr, sizeof(LXHeader));

  // Verify magic
  if (_header.magic == kLXMagic) {
    _isLE = false;
    DS2LOG(Debug, "detected LX format");
  } else if (_header.magic == kLEMagic) {
    _isLE = true;
    DS2LOG(Debug, "detected LE format");
  } else {
    DS2LOG(Error, "invalid LX/LE magic: 0x%04x", _header.magic);
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug,
         "LX/LE: objects=%u, eipObj=%u, eip=0x%x, espObj=%u, esp=0x%x",
         _header.objectCount, _header.eipObject, _header.eip,
         _header.espObject, _header.esp);

  // Read object table
  _objects.clear();
  uint32_t objTableOffset = lxOffset + _header.objectTableOffset;

  if (objTableOffset + _header.objectCount * sizeof(LXObject) > size) {
    DS2LOG(Error, "object table extends beyond file");
    return kErrorInvalidArgument;
  }

  ptr = static_cast<const uint8_t *>(data) + objTableOffset;
  for (uint32_t i = 0; i < _header.objectCount; i++) {
    LXObject obj;
    memcpy(&obj, ptr, sizeof(LXObject));
    ptr += sizeof(LXObject);

    _objects.push_back(obj);

    DS2LOG(Debug,
           "Object %u: size=0x%x, base=0x%x, flags=0x%x, pages=%u",
           i + 1, obj.virtualSize, obj.relocBaseAddr, obj.objectFlags,
           obj.pageTableEntries);
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode LXLoader::loadObject(ProcessBase *process, size_t objectIndex,
                                Address baseAddress, Address &objectAddress) {
  if (objectIndex >= _objects.size())
    return kErrorInvalidArgument;

  const LXObject &obj = _objects[objectIndex];

  // Calculate object load address
  objectAddress = baseAddress + obj.relocBaseAddr;

  // Allocate memory for object
  uint32_t protection = 0;
  if (obj.objectFlags & kLXObjectReadable)
    protection |= 0x01;
  if (obj.objectFlags & kLXObjectWritable)
    protection |= 0x02;
  if (obj.objectFlags & kLXObjectExecutable)
    protection |= 0x04;

  ErrorCode error =
      process->allocateMemory(objectAddress, obj.virtualSize, protection);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to allocate memory for object %zu", objectIndex);
    return error;
  }

  // Load pages for this object
  for (uint32_t page = 0; page < obj.pageTableEntries; page++) {
    uint32_t pageIndex = obj.pageTableIndex + page;
    Address pageAddress = objectAddress + (page * _header.pageSize);

    error = loadPage(process, pageIndex, pageAddress);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to load page %u for object %zu", page,
             objectIndex);
      // Continue loading other pages
    }
  }

  return kSuccess;
}

ErrorCode LXLoader::loadPage(ProcessBase *process, uint32_t pageIndex,
                              Address address) {
  // Read page table entry
  uint32_t pageTableOffset = _header.objectPageTableOffset;
  if (pageTableOffset + (pageIndex + 1) * sizeof(LXPageTableEntry) >
      _data.size()) {
    return kErrorInvalidArgument;
  }

  LXPageTableEntry pageEntry;
  const uint8_t *ptr = _data.data() + pageTableOffset +
                       pageIndex * sizeof(LXPageTableEntry);
  memcpy(&pageEntry, ptr, sizeof(LXPageTableEntry));

  // Extract page data offset (low 24 bits) and flags (high 8 bits)
  uint32_t pageDataOffset = pageEntry.pageDataOffset & 0x00FFFFFF;
  uint8_t pageType = (pageEntry.pageDataOffset >> 24) & 0xFF;

  // Page types:
  // 0 = legal physical page
  // 1 = iterated data page
  // 2 = invalid page
  // 3 = zero-filled page
  // 4 = range of pages

  if (pageType == 3 || (pageEntry.dataSize == 0)) {
    // Zero-filled page
    std::vector<uint8_t> zeros(_header.pageSize, 0);
    return process->writeMemory(address, zeros.data(), _header.pageSize,
                               nullptr);
  } else if (pageType == 0) {
    // Legal physical page - load from file
    uint32_t dataOffset = _header.dataPages + pageDataOffset;
    if (dataOffset + pageEntry.dataSize > _data.size()) {
      DS2LOG(Error, "page data extends beyond file");
      return kErrorInvalidArgument;
    }

    const uint8_t *pageData = _data.data() + dataOffset;
    return process->writeMemory(address, pageData, pageEntry.dataSize,
                               nullptr);
  } else if (pageType == 1) {
    // Iterated data page (compressed)
    // Would need to implement decompression
    DS2LOG(Warning, "iterated data pages not fully supported");
    return kErrorUnsupported;
  }

  return kSuccess;
}

ErrorCode LXLoader::processFixups(ProcessBase *process, Address baseAddress) {
  // LX fixups are complex with multiple types
  // This is a placeholder for fixup processing
  DS2LOG(Debug, "processing LX fixups");
  return kSuccess;
}

ErrorCode LXLoader::getObjects(std::vector<Section> &objects) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  objects.clear();
  for (const auto &obj : _objects) {
    Section section;
    section.start = obj.relocBaseAddr;
    section.length = obj.virtualSize;
    objects.push_back(section);
  }

  return kSuccess;
}

ErrorCode LXLoader::getImportedModules(std::vector<std::string> &modules) const {
  // Would parse import module table
  modules.clear();
  return kSuccess;
}

} // namespace OS2
} // namespace Target
} // namespace ds2
