//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/SOM/Loader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace SOM {

Loader::Loader() : _parsed(false), _is64Bit(false) {
  memset(&_header, 0, sizeof(_header));
}

Loader::~Loader() {}

ErrorCode Loader::load(ProcessBase *process, std::string const &path,
                       Address &loadAddress, Address &entryPoint) {
  // Read the SOM file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open SOM file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read SOM file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse headers
  ErrorCode error = parseHeaders(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address (PA-RISC typically loads at 0x40000000)
  Address baseAddress = 0x40000000;
  loadAddress = baseAddress;

  // Load all loadable subspaces
  for (size_t i = 0; i < _subspaces.size(); i++) {
    const SOMSubspace &subspace = _subspaces[i];

    // Skip non-loadable subspaces
    if (!(subspace.flags & kSubspaceLoadable))
      continue;

    Address subspaceAddress;
    error = loadSubspace(process, i, baseAddress, subspaceAddress);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to load subspace %zu", i);
      return error;
    }
  }

  // Process fixups (relocations)
  error = processFixups(process, baseAddress);
  if (error != kSuccess) {
    DS2LOG(Warning, "failed to process fixups");
  }

  // Calculate entry point
  if (_is64Bit) {
    uint32_t entrySpaceIdx = _header.header64.entry_space;
    uint32_t entrySubspaceIdx = _header.header64.entry_subspace;
    uint64_t entryOffset = _header.header64.entry_offset;

    if (entrySubspaceIdx < _subspaces.size()) {
      const SOMSubspace &entrySub = _subspaces[entrySubspaceIdx];
      entryPoint = baseAddress + entrySub.subspace_start + entryOffset;
    } else {
      entryPoint = baseAddress + entryOffset;
    }
  } else {
    uint32_t entrySpaceIdx = _header.header32.entry_space;
    uint32_t entrySubspaceIdx = _header.header32.entry_subspace;
    uint32_t entryOffset = _header.header32.entry_offset;

    if (entrySubspaceIdx < _subspaces.size()) {
      const SOMSubspace &entrySub = _subspaces[entrySubspaceIdx];
      entryPoint = baseAddress + entrySub.subspace_start + entryOffset;
    } else {
      entryPoint = baseAddress + entryOffset;
    }
  }

  DS2LOG(Debug, "SOM loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode Loader::parseHeaders(void const *data, size_t size) {
  if (size < sizeof(SOMHeader32)) {
    DS2LOG(Error, "file too small for SOM header");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  // Read system_id and magic to determine format
  uint16_t system_id, magic;
  memcpy(&system_id, ptr, sizeof(system_id));
  memcpy(&magic, ptr + 2, sizeof(magic));

  // Determine if 32-bit or 64-bit
  if (magic == kSOMMagic64Exec || magic == kSOMMagic64Shared) {
    _is64Bit = true;
    if (size < sizeof(SOMHeader64)) {
      DS2LOG(Error, "file too small for SOM64 header");
      return kErrorInvalidArgument;
    }
    memcpy(&_header.header64, ptr, sizeof(SOMHeader64));
    DS2LOG(Debug, "SOM64 format detected");
  } else if (magic == kSOMMagic32Exec || magic == kSOMMagic32Shared) {
    _is64Bit = false;
    memcpy(&_header.header32, ptr, sizeof(SOMHeader32));
    DS2LOG(Debug, "SOM32 format detected");
  } else {
    DS2LOG(Error, "invalid SOM magic: 0x%04x", magic);
    return kErrorInvalidArgument;
  }

  // Verify system ID
  if (system_id != kSystemIDHPUX && system_id != kSystemIDHPUX64 &&
      system_id != kSystemIDMPE) {
    DS2LOG(Warning, "unknown system ID: 0x%04x", system_id);
  }

  // Read space dictionary
  _spaces.clear();
  uint64_t spaceLocation, spaceTotal;

  if (_is64Bit) {
    spaceLocation = _header.header64.space_location;
    spaceTotal = _header.header64.space_total;
  } else {
    spaceLocation = _header.header32.space_location;
    spaceTotal = _header.header32.space_total;
  }

  if (spaceLocation > 0 && spaceTotal > 0) {
    if (spaceLocation + spaceTotal * sizeof(SOMSpace) > size) {
      DS2LOG(Error, "space dictionary extends beyond file");
      return kErrorInvalidArgument;
    }

    ptr = static_cast<const uint8_t *>(data) + spaceLocation;
    for (uint64_t i = 0; i < spaceTotal; i++) {
      SOMSpace space;
      memcpy(&space, ptr, sizeof(SOMSpace));
      ptr += sizeof(SOMSpace);
      _spaces.push_back(space);
    }

    DS2LOG(Debug, "loaded %llu spaces", (unsigned long long)spaceTotal);
  }

  // Read subspace dictionary
  _subspaces.clear();
  uint64_t subspaceLocation, subspaceTotal;

  if (_is64Bit) {
    subspaceLocation = _header.header64.subspace_location;
    subspaceTotal = _header.header64.subspace_total;
  } else {
    subspaceLocation = _header.header32.subspace_location;
    subspaceTotal = _header.header32.subspace_total;
  }

  if (subspaceLocation > 0 && subspaceTotal > 0) {
    if (subspaceLocation + subspaceTotal * sizeof(SOMSubspace) > size) {
      DS2LOG(Error, "subspace dictionary extends beyond file");
      return kErrorInvalidArgument;
    }

    ptr = static_cast<const uint8_t *>(data) + subspaceLocation;
    for (uint64_t i = 0; i < subspaceTotal; i++) {
      SOMSubspace subspace;
      memcpy(&subspace, ptr, sizeof(SOMSubspace));
      ptr += sizeof(SOMSubspace);
      _subspaces.push_back(subspace);

      DS2LOG(Debug,
             "Subspace %llu: flags=0x%x, start=0x%x, length=0x%x, "
             "file_loc=0x%x",
             (unsigned long long)i, subspace.flags, subspace.subspace_start,
             subspace.subspace_length, subspace.file_loc_init_value);
    }

    DS2LOG(Debug, "loaded %llu subspaces", (unsigned long long)subspaceTotal);
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode Loader::loadSubspace(ProcessBase *process, size_t subspaceIndex,
                                Address baseAddress,
                                Address &subspaceAddress) {
  if (subspaceIndex >= _subspaces.size())
    return kErrorInvalidArgument;

  const SOMSubspace &subspace = _subspaces[subspaceIndex];

  // Calculate subspace address
  subspaceAddress = baseAddress + subspace.subspace_start;

  // Determine protection flags
  uint32_t protection = 0;
  if (subspace.flags & kSubspaceCode)
    protection = 0x05; // Read + Execute
  else if (subspace.flags & kSubspaceData)
    protection = 0x03; // Read + Write
  else
    protection = 0x01; // Read only

  // Allocate memory for subspace
  ErrorCode error = process->allocateMemory(subspaceAddress,
                                           subspace.subspace_length, protection);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to allocate memory for subspace %zu", subspaceIndex);
    return error;
  }

  // Load subspace data
  if (subspace.file_loc_init_value > 0 &&
      subspace.initialization_length > 0) {
    if (subspace.file_loc_init_value + subspace.initialization_length >
        _data.size()) {
      DS2LOG(Error, "subspace data extends beyond file");
      return kErrorInvalidArgument;
    }

    const uint8_t *subspaceData = _data.data() + subspace.file_loc_init_value;

    error = process->writeMemory(subspaceAddress, subspaceData,
                                 subspace.initialization_length, nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to write subspace %zu data", subspaceIndex);
      return error;
    }
  } else if (subspace.flags & kSubspaceZeroFill) {
    // Zero-fill subspace
    std::vector<uint8_t> zeros(subspace.subspace_length, 0);
    error = process->writeMemory(subspaceAddress, zeros.data(),
                                 subspace.subspace_length, nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to zero-fill subspace %zu", subspaceIndex);
      return error;
    }
  }

  return kSuccess;
}

ErrorCode Loader::processFixups(ProcessBase *process, Address baseAddress) {
  // SOM fixups are complex and would require extensive implementation
  // This is a placeholder for fixup processing
  DS2LOG(Debug, "processing SOM fixups");
  return kSuccess;
}

std::string Loader::readString(uint32_t offset) const {
  if (!_parsed)
    return "";

  uint64_t stringLocation, stringSize;

  if (_is64Bit) {
    stringLocation = _header.header64.string_location;
    stringSize = _header.header64.string_size;
  } else {
    stringLocation = _header.header32.string_location;
    stringSize = _header.header32.string_size;
  }

  if (offset >= stringSize)
    return "";

  const char *strData =
      reinterpret_cast<const char *>(_data.data() + stringLocation + offset);
  return std::string(strData);
}

bool Loader::isSharedLibrary() const {
  if (!_parsed)
    return false;

  uint16_t magic = _is64Bit ? _header.header64.a_magic : _header.header32.a_magic;
  return (magic == kSOMMagic32Shared || magic == kSOMMagic64Shared);
}

ErrorCode Loader::getSpaces(std::vector<SOMSpace> &spaces) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  spaces = _spaces;
  return kSuccess;
}

ErrorCode Loader::getSubspaces(std::vector<SOMSubspace> &subspaces) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  subspaces = _subspaces;
  return kSuccess;
}

ErrorCode Loader::getSections(std::vector<Section> &sections) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  sections.clear();
  for (const auto &subspace : _subspaces) {
    if (subspace.flags & kSubspaceLoadable) {
      Section section;
      section.start = subspace.subspace_start;
      section.length = subspace.subspace_length;
      sections.push_back(section);
    }
  }

  return kSuccess;
}

} // namespace SOM
} // namespace Target
} // namespace ds2
