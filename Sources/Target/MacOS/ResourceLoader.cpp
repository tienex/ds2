//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/MacOS/ResourceLoader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace MacOS {

ResourceLoader::ResourceLoader()
    : _parsed(false), _is68k(false), _isPowerPC(false) {
  memset(&_jumpTable, 0, sizeof(_jumpTable));
}

ResourceLoader::~ResourceLoader() {}

uint16_t ResourceLoader::readBE16(const uint8_t *&ptr) {
  uint16_t value = (ptr[0] << 8) | ptr[1];
  ptr += 2;
  return value;
}

uint32_t ResourceLoader::readBE32(const uint8_t *&ptr) {
  uint32_t value = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  return value;
}

std::string ResourceLoader::readPString(const uint8_t *ptr) {
  uint8_t len = ptr[0];
  return std::string(reinterpret_cast<const char *>(ptr + 1), len);
}

ErrorCode ResourceLoader::load(ProcessBase *process, std::string const &path,
                               Address &loadAddress, Address &entryPoint) {
  // Classic Mac OS files have resource fork and data fork
  // For simplicity, we'll try to read the resource fork
  // In real Mac OS, this would use Resource Manager APIs

  // Try to open resource fork (path + "/..namedfork/rsrc" on macOS)
  std::string rsrcPath = path + "/..namedfork/rsrc";
  std::ifstream file(rsrcPath, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    // Try AppleDouble format (._filename)
    size_t lastSlash = path.find_last_of('/');
    std::string dir = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
    std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    rsrcPath = dir + "._" + name;
    file.open(rsrcPath, std::ios::binary | std::ios::ate);
  }

  if (!file.is_open()) {
    // Try as raw resource fork data
    file.open(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      DS2LOG(Error, "failed to open Mac resource file: %s", path.c_str());
      return kErrorInvalidArgument;
    }
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read Mac resource file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse resource fork
  ErrorCode error = parseResourceFork(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address (Classic Mac OS apps typically use A5 world)
  Address baseAddress = 0x00001000; // Typical application base
  loadAddress = baseAddress;

  // Check if this is a PowerPC app (has 'cfrg' resource)
  Resource cfrgResource;
  if (getResource(kPEFResource, 0, cfrgResource) == kSuccess) {
    _isPowerPC = true;
    error = loadPEFContainer(process, baseAddress, entryPoint);
  } else if (_resources.find(kCodeResource) != _resources.end()) {
    _is68k = true;
    error = loadCodeResources(process, baseAddress, entryPoint);
  } else {
    DS2LOG(Error, "no CODE or cfrg resources found");
    return kErrorInvalidArgument;
  }

  if (error != kSuccess)
    return error;

  DS2LOG(Debug, "Mac resource executable loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode ResourceLoader::parseResourceFork(void const *data, size_t size) {
  if (size < sizeof(ResourceHeader)) {
    DS2LOG(Error, "file too small for resource header");
    return kErrorInvalidArgument;
  }

  const uint8_t *base = static_cast<const uint8_t *>(data);
  const uint8_t *ptr = base;

  // Read resource header (big-endian)
  ResourceHeader header;
  header.dataOffset = readBE32(ptr);
  header.mapOffset = readBE32(ptr);
  header.dataLength = readBE32(ptr);
  header.mapLength = readBE32(ptr);

  DS2LOG(Debug, "Resource fork: data=0x%x, map=0x%x, dataLen=0x%x, mapLen=0x%x",
         header.dataOffset, header.mapOffset, header.dataLength,
         header.mapLength);

  // Read resource map
  if (header.mapOffset + header.mapLength > size) {
    DS2LOG(Error, "resource map extends beyond file");
    return kErrorInvalidArgument;
  }

  ptr = base + header.mapOffset;
  ptr += 16; // Skip duplicate header and handles

  // Read map header
  uint16_t resourceForkAttrs = readBE16(ptr);
  uint16_t typeListOffset = readBE16(ptr);
  uint16_t nameListOffset = readBE16(ptr);

  ptr = base + header.mapOffset + typeListOffset;
  uint16_t numTypesMinusOne = readBE16(ptr);
  uint16_t numTypes = numTypesMinusOne + 1;

  DS2LOG(Debug, "Resource map: %u types", numTypes);

  // Read type list
  for (uint16_t i = 0; i < numTypes; i++) {
    uint32_t type = readBE32(ptr);
    uint16_t numResourcesMinusOne = readBE16(ptr);
    uint16_t refListOffset = readBE16(ptr);

    uint16_t numResources = numResourcesMinusOne + 1;

    // Read reference list for this type
    const uint8_t *refPtr =
        base + header.mapOffset + typeListOffset + refListOffset;

    for (uint16_t j = 0; j < numResources; j++) {
      Resource resource;
      resource.type = type;
      resource.id = readBE16(refPtr);

      int16_t nameOffset = readBE16(refPtr);
      resource.attributes = *refPtr++;

      // Read 3-byte data offset
      uint32_t dataOffset = (*refPtr++ << 16);
      dataOffset |= readBE16(refPtr);

      refPtr += 4; // Skip handle placeholder

      // Read resource name if present
      if (nameOffset != 0xFFFF) {
        const uint8_t *namePtr =
            base + header.mapOffset + nameListOffset + nameOffset;
        resource.name = readPString(namePtr);
      }

      // Read resource data
      if (header.dataOffset + dataOffset + 4 <= size) {
        const uint8_t *dataPtr = base + header.dataOffset + dataOffset;
        uint32_t dataLength = readBE32(dataPtr);

        if (header.dataOffset + dataOffset + 4 + dataLength <= size) {
          resource.data.resize(dataLength);
          memcpy(resource.data.data(), dataPtr, dataLength);

          _resources[type][resource.id] = resource;

          DS2LOG(Debug, "Resource '%c%c%c%c' ID=%d, size=%u, name='%s'",
                 (type >> 24) & 0xFF, (type >> 16) & 0xFF,
                 (type >> 8) & 0xFF, type & 0xFF,
                 resource.id, dataLength, resource.name.c_str());
        }
      }
    }
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode ResourceLoader::loadCodeResources(ProcessBase *process,
                                           Address baseAddress,
                                           Address &entryPoint) {
  // Load CODE 0 (jump table)
  Resource code0;
  if (getResource(kCodeResource, 0, code0) != kSuccess) {
    DS2LOG(Error, "CODE 0 resource not found");
    return kErrorInvalidArgument;
  }

  if (code0.data.size() < sizeof(JumpTableHeader)) {
    DS2LOG(Error, "CODE 0 too small");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = code0.data.data();
  _jumpTable.aboveA5Size = readBE16(ptr);
  _jumpTable.appParams = readBE32(ptr);
  _jumpTable.jumpTableSize = readBE32(ptr);
  _jumpTable.jumpTableOffset = readBE32(ptr);

  DS2LOG(Debug, "Jump table: above A5=%u, size=%u, offset=%u",
         _jumpTable.aboveA5Size, _jumpTable.jumpTableSize,
         _jumpTable.jumpTableOffset);

  // Calculate A5 (points to boundary between globals and jump table)
  Address a5 = baseAddress + 0x10000; // Typical A5 location
  Address jumpTableAddr = a5 - _jumpTable.jumpTableOffset;

  // Set up jump table
  ErrorCode error = setupJumpTable(process, a5);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to set up jump table");
    return error;
  }

  // Load all CODE resources (CODE 1, CODE 2, etc.)
  Address currentCodeAddr = baseAddress + 0x20000; // Start after A5 world

  if (_resources.find(kCodeResource) != _resources.end()) {
    for (auto &entry : _resources[kCodeResource]) {
      uint16_t codeID = entry.first;

      // Skip CODE 0 (already processed)
      if (codeID == 0)
        continue;

      Resource &code = entry.second;

      if (code.data.size() < sizeof(CodeResourceHeader)) {
        DS2LOG(Warning, "CODE %u too small", codeID);
        continue;
      }

      // Parse CODE resource header
      const uint8_t *codePtr = code.data.data();
      uint16_t entryOffset = readBE16(codePtr);
      uint16_t numJTEntries = readBE16(codePtr);

      uint32_t codeSize = code.data.size() - 4; // Subtract header

      // Allocate memory for CODE segment
      error = process->allocateMemory(currentCodeAddr, codeSize,
                                     0x05); // Read + Execute
      if (error != kSuccess) {
        DS2LOG(Error, "failed to allocate CODE %u", codeID);
        return error;
      }

      // Write CODE data (skip header)
      error = process->writeMemory(currentCodeAddr, codePtr, codeSize,
                                   nullptr);
      if (error != kSuccess) {
        DS2LOG(Error, "failed to write CODE %u", codeID);
        return error;
      }

      DS2LOG(Debug, "Loaded CODE %u at 0x%llx, size=%u, entry offset=%u",
             codeID, (unsigned long long)currentCodeAddr.value(),
             codeSize, entryOffset);

      // Entry point is in CODE 1
      if (codeID == 1) {
        entryPoint = currentCodeAddr + entryOffset;
      }

      currentCodeAddr += (codeSize + 3) & ~3; // 4-byte align
    }
  }

  if (entryPoint.value() == 0) {
    DS2LOG(Error, "no entry point found (CODE 1 missing)");
    return kErrorInvalidArgument;
  }

  return kSuccess;
}

ErrorCode ResourceLoader::loadPEFContainer(ProcessBase *process,
                                          Address baseAddress,
                                          Address &entryPoint) {
  // Get 'cfrg' resource which contains PEF container reference
  Resource cfrg;
  if (getResource(kPEFResource, 0, cfrg) != kSuccess) {
    DS2LOG(Error, "cfrg resource not found");
    return kErrorInvalidArgument;
  }

  // The 'cfrg' resource points to the actual PEF container
  // For simplicity, we'll look for 'CODE' resource ID 0 which often contains
  // the PEF data on PowerPC apps
  Resource pefData;
  if (getResource(kCodeResource, 0, pefData) != kSuccess) {
    DS2LOG(Error, "PEF container not found");
    return kErrorInvalidArgument;
  }

  if (pefData.data.size() < sizeof(PEFContainerHeader)) {
    DS2LOG(Error, "PEF container too small");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = pefData.data.data();

  // Read PEF container header
  PEFContainerHeader header;
  header.tag1 = readBE32(ptr);
  header.tag2 = readBE32(ptr);

  // Check magic numbers
  if (header.tag1 != 0x4A6F7921 || header.tag2 != 0x70656666) { // 'Joy!' 'peff'
    DS2LOG(Error, "invalid PEF magic: 0x%08x 0x%08x", header.tag1, header.tag2);
    return kErrorInvalidArgument;
  }

  header.architecture = readBE32(ptr);
  header.formatVersion = readBE32(ptr);
  header.dateTimeStamp = readBE32(ptr);
  header.oldDefVersion = readBE32(ptr);
  header.oldImpVersion = readBE32(ptr);
  header.currentVersion = readBE32(ptr);
  header.sectionCount = readBE16(ptr);
  header.instSectionCount = readBE16(ptr);
  header.reservedA = readBE32(ptr);

  DS2LOG(Debug, "PEF: arch=0x%x, version=%u, sections=%u",
         header.architecture, header.formatVersion, header.sectionCount);

  // Load PEF sections
  Address currentAddr = baseAddress;

  for (uint16_t i = 0; i < header.sectionCount; i++) {
    if (ptr + sizeof(PEFSectionHeader) > pefData.data.data() + pefData.data.size()) {
      DS2LOG(Warning, "PEF section header %u truncated", i);
      break;
    }

    PEFSectionHeader section;
    section.nameOffset = readBE32(ptr);
    section.defaultAddress = readBE32(ptr);
    section.totalLength = readBE32(ptr);
    section.unpackedLength = readBE32(ptr);
    section.containerLength = readBE32(ptr);
    section.containerOffset = readBE32(ptr);
    section.sectionKind = *ptr++;
    section.shareKind = *ptr++;
    section.alignment = *ptr++;
    section.reserved = *ptr++;

    // Determine protection
    uint32_t protection = 0x01; // Read
    if (section.sectionKind == 0) // Code section
      protection = 0x05; // Read + Execute
    else if (section.sectionKind == 1) // Data section
      protection = 0x03; // Read + Write

    // Allocate memory
    ErrorCode error = process->allocateMemory(currentAddr, section.totalLength,
                                             protection);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to allocate PEF section %u", i);
      return error;
    }

    // Load section data
    if (section.containerLength > 0 &&
        section.containerOffset + section.containerLength <= pefData.data.size()) {
      const uint8_t *sectionData = pefData.data.data() + section.containerOffset;

      error = process->writeMemory(currentAddr, sectionData,
                                   section.containerLength, nullptr);
      if (error != kSuccess) {
        DS2LOG(Error, "failed to write PEF section %u", i);
        return error;
      }
    }

    DS2LOG(Debug, "PEF section %u: kind=%u, addr=0x%llx, size=%u",
           i, section.sectionKind, (unsigned long long)currentAddr.value(),
           section.totalLength);

    // First code section is entry point
    if (i == 0 && section.sectionKind == 0) {
      entryPoint = currentAddr;
    }

    currentAddr += (section.totalLength + 15) & ~15; // 16-byte align
  }

  return kSuccess;
}

ErrorCode ResourceLoader::setupJumpTable(ProcessBase *process, Address a5) {
  // The jump table is located below A5
  // Each entry is 8 bytes: instruction sequence to jump to CODE segment

  // For now, we'll just allocate the space
  // A real implementation would patch the jump table entries

  uint32_t jumpTableSize = _jumpTable.jumpTableSize;
  Address jumpTableAddr = a5 - _jumpTable.jumpTableOffset;

  ErrorCode error = process->allocateMemory(jumpTableAddr, jumpTableSize,
                                           0x03); // Read + Write

  if (error != kSuccess) {
    DS2LOG(Error, "failed to allocate jump table");
    return error;
  }

  // Zero-fill jump table
  std::vector<uint8_t> zeros(jumpTableSize, 0);
  error = process->writeMemory(jumpTableAddr, zeros.data(), jumpTableSize,
                              nullptr);

  return error;
}

ErrorCode ResourceLoader::getResources(
    uint32_t type, std::vector<Resource> &resources) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  auto typeIt = _resources.find(type);
  if (typeIt == _resources.end())
    return kErrorInvalidArgument;

  resources.clear();
  for (const auto &entry : typeIt->second) {
    resources.push_back(entry.second);
  }

  return kSuccess;
}

ErrorCode ResourceLoader::getResource(uint32_t type, uint16_t id,
                                      Resource &resource) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  auto typeIt = _resources.find(type);
  if (typeIt == _resources.end())
    return kErrorInvalidArgument;

  auto idIt = typeIt->second.find(id);
  if (idIt == typeIt->second.end())
    return kErrorInvalidArgument;

  resource = idIt->second;
  return kSuccess;
}

ErrorCode ResourceLoader::getMemoryRequirements(uint32_t &preferredSize,
                                                uint32_t &minimumSize) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  Resource sizeResource;
  if (getResource(kSizeResource, -1, sizeResource) != kSuccess) {
    // No SIZE resource, use defaults
    preferredSize = 512 * 1024;  // 512 KB
    minimumSize = 256 * 1024;    // 256 KB
    return kSuccess;
  }

  if (sizeResource.data.size() < sizeof(SizeResource)) {
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = sizeResource.data.data();
  uint16_t flags = readBE16(ptr);
  preferredSize = readBE32(ptr);
  minimumSize = readBE32(ptr);

  return kSuccess;
}

} // namespace MacOS
} // namespace Target
} // namespace ds2
