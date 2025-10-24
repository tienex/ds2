//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Amiga/HunkLoader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace Amiga {

HunkLoader::HunkLoader() : _parsed(false) {}

HunkLoader::~HunkLoader() {}

uint32_t HunkLoader::readBE32(const uint8_t *&ptr) {
  uint32_t value = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  ptr += 4;
  return value;
}

std::string HunkLoader::readString(const uint8_t *&ptr) {
  uint32_t len = readBE32(ptr);
  len *= 4; // Length in longwords, convert to bytes

  std::string str(reinterpret_cast<const char *>(ptr), len);
  ptr += len;

  // Remove padding nulls
  size_t nullPos = str.find('\0');
  if (nullPos != std::string::npos)
    str.resize(nullPos);

  return str;
}

ErrorCode HunkLoader::load(ProcessBase *process, std::string const &path,
                           Address &loadAddress, Address &entryPoint) {
  // Read the Hunk file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open Hunk file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read Hunk file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse hunks
  ErrorCode error = parseHunks(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address (Amiga programs typically start at 0x4)
  Address baseAddress = 0x1000;
  loadAddress = baseAddress;

  // Load all hunks
  Address currentAddress = baseAddress;
  for (size_t i = 0; i < _hunks.size(); i++) {
    Hunk &hunk = _hunks[i];
    hunk.loadAddress = currentAddress;

    error = loadHunk(process, i, currentAddress);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to load hunk %zu", i);
      return error;
    }

    // Align to next boundary
    currentAddress = hunk.loadAddress + hunk.size * 4;
    currentAddress = (currentAddress + 3) & ~3; // 4-byte align
  }

  // Process relocations
  for (size_t i = 0; i < _hunks.size(); i++) {
    error = processHunkRelocations(process, i);
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to process relocations for hunk %zu", i);
    }
  }

  // Entry point is at start of first code hunk
  entryPoint = baseAddress;
  for (const auto &hunk : _hunks) {
    if (hunk.type == kHunkCode) {
      entryPoint = hunk.loadAddress;
      break;
    }
  }

  DS2LOG(Debug, "Hunk loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode HunkLoader::parseHunks(void const *data, size_t size) {
  const uint8_t *ptr = static_cast<const uint8_t *>(data);
  const uint8_t *end = ptr + size;

  // Read header hunk
  uint32_t hunkType = readBE32(ptr);
  if (hunkType != kHunkHeader) {
    DS2LOG(Error, "expected HUNK_HEADER, got 0x%x", hunkType);
    return kErrorInvalidArgument;
  }

  // Read resident library names (if any)
  uint32_t nameCount = 0;
  while (ptr < end) {
    uint32_t nameLen = readBE32(ptr);
    if (nameLen == 0)
      break;

    // Skip name
    ptr += nameLen * 4;
    nameCount++;
  }

  // Read number of hunks
  uint32_t numHunks = readBE32(ptr);
  uint32_t firstHunk = readBE32(ptr);
  uint32_t lastHunk = readBE32(ptr);

  DS2LOG(Debug, "Hunk file: %u hunks (%u-%u)", numHunks, firstHunk, lastHunk);

  // Read hunk sizes
  std::vector<uint32_t> hunkSizes;
  for (uint32_t i = 0; i <= (lastHunk - firstHunk); i++) {
    uint32_t size = readBE32(ptr);
    
    // Extract memory type from flags
    uint32_t memType = 0;
    if (size & kHunkFlagChip)
      memType = kMemTypeChip;
    else if (size & kHunkFlagFast)
      memType = kMemTypeFast;
    
    size &= 0x3FFFFFFF; // Mask off memory type bits
    hunkSizes.push_back(size);
  }

  // Read hunks
  _hunks.clear();
  _relocations.clear();
  _relocations.resize(numHunks);

  while (ptr < end) {
    uint32_t type = readBE32(ptr);

    // Check for end of file
    if (type == 0)
      break;

    switch (type) {
    case kHunkCode:
    case kHunkData:
    case kHunkBSS: {
      Hunk hunk;
      hunk.type = type;
      hunk.size = readBE32(ptr);
      hunk.memType = kMemTypeAny;

      if (type != kHunkBSS) {
        // Read hunk data
        uint32_t dataSize = hunk.size * 4;
        hunk.data.resize(dataSize);
        if (ptr + dataSize > end) {
          DS2LOG(Error, "hunk data extends beyond file");
          return kErrorInvalidArgument;
        }
        memcpy(hunk.data.data(), ptr, dataSize);
        ptr += dataSize;
      }

      _hunks.push_back(hunk);
      DS2LOG(Debug, "Hunk %zu: type=0x%x, size=%u",
             _hunks.size() - 1, type, hunk.size);
      break;
    }

    case kHunkReloc32:
    case kHunkReloc16:
    case kHunkReloc8:
    case kHunkDReloc32:
    case kHunkDReloc16:
    case kHunkDReloc8:
    case kHunkReloc32Short: {
      size_t currentHunk = _hunks.size() - 1;
      
      while (true) {
        uint32_t numOffsets = readBE32(ptr);
        if (numOffsets == 0)
          break;

        uint32_t targetHunk = readBE32(ptr);

        HunkRelocation reloc;
        reloc.targetHunk = targetHunk;

        for (uint32_t i = 0; i < numOffsets; i++) {
          uint32_t offset = readBE32(ptr);
          reloc.offsets.push_back(offset);
        }

        _relocations[currentHunk].push_back(reloc);
      }
      break;
    }

    case kHunkSymbol:
    case kHunkDebug:
    case kHunkName: {
      // Skip these hunks for now
      uint32_t size = readBE32(ptr);
      ptr += size * 4;
      break;
    }

    case kHunkEnd:
      // End of current hunk
      break;

    default:
      DS2LOG(Warning, "unknown hunk type: 0x%x", type);
      break;
    }
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode HunkLoader::loadHunk(ProcessBase *process, size_t hunkIndex,
                               Address currentAddress) {
  if (hunkIndex >= _hunks.size())
    return kErrorInvalidArgument;

  Hunk &hunk = _hunks[hunkIndex];
  uint32_t hunkSize = hunk.size * 4;

  // Determine protection flags
  uint32_t protection = 0x01; // Read
  if (hunk.type == kHunkCode)
    protection = 0x05; // Read + Execute
  else if (hunk.type == kHunkData || hunk.type == kHunkBSS)
    protection = 0x03; // Read + Write

  // Allocate memory
  ErrorCode error = process->allocateMemory(currentAddress, hunkSize,
                                           protection);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to allocate memory for hunk %zu", hunkIndex);
    return error;
  }

  // Write hunk data
  if (hunk.type != kHunkBSS && !hunk.data.empty()) {
    error = process->writeMemory(currentAddress, hunk.data.data(),
                                 hunk.data.size(), nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to write hunk %zu data", hunkIndex);
      return error;
    }
  } else if (hunk.type == kHunkBSS) {
    // Zero-fill BSS
    std::vector<uint8_t> zeros(hunkSize, 0);
    error = process->writeMemory(currentAddress, zeros.data(), hunkSize,
                                 nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to zero-fill BSS hunk %zu", hunkIndex);
      return error;
    }
  }

  return kSuccess;
}

ErrorCode HunkLoader::processHunkRelocations(ProcessBase *process,
                                             size_t hunkIndex) {
  if (hunkIndex >= _relocations.size())
    return kSuccess;

  const Hunk &sourceHunk = _hunks[hunkIndex];

  for (const auto &reloc : _relocations[hunkIndex]) {
    if (reloc.targetHunk >= _hunks.size()) {
      DS2LOG(Warning, "invalid target hunk: %u", reloc.targetHunk);
      continue;
    }

    const Hunk &targetHunk = _hunks[reloc.targetHunk];

    for (uint32_t offset : reloc.offsets) {
      Address relocAddr = sourceHunk.loadAddress + offset;

      // Read current value
      uint32_t value;
      ErrorCode error = process->readMemory(relocAddr, &value, sizeof(value),
                                           nullptr);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed to read relocation at offset 0x%x", offset);
        continue;
      }

      // Convert from big-endian, add target hunk base, convert back
      value = __builtin_bswap32(value);
      value += targetHunk.loadAddress.value();
      value = __builtin_bswap32(value);

      error = process->writeMemory(relocAddr, &value, sizeof(value), nullptr);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed to write relocation at offset 0x%x", offset);
        continue;
      }
    }
  }

  return kSuccess;
}

bool HunkLoader::requiresChipMem() const {
  for (const auto &hunk : _hunks) {
    if (hunk.memType == kMemTypeChip)
      return true;
  }
  return false;
}

bool HunkLoader::requiresFastMem() const {
  for (const auto &hunk : _hunks) {
    if (hunk.memType == kMemTypeFast)
      return true;
  }
  return false;
}

ErrorCode HunkLoader::getHunks(std::vector<Hunk> &hunks) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  hunks = _hunks;
  return kSuccess;
}

} // namespace Amiga
} // namespace Target
} // namespace ds2
