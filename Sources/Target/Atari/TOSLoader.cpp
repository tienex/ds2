//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Atari/TOSLoader.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <fstream>

namespace ds2 {
namespace Target {
namespace Atari {

TOSLoader::TOSLoader()
    : _parsed(false), _hasRelocations(false), _isExtended(false) {
  memset(&_header, 0, sizeof(_header));
  memset(&_extHeader, 0, sizeof(_extHeader));
}

TOSLoader::~TOSLoader() {}

ErrorCode TOSLoader::load(ProcessBase *process, std::string const &path,
                          Address &loadAddress, Address &entryPoint) {
  // Read the TOS file
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    DS2LOG(Error, "failed to open TOS file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  _data.resize(size);
  if (!file.read(reinterpret_cast<char *>(_data.data()), size)) {
    DS2LOG(Error, "failed to read TOS file: %s", path.c_str());
    return kErrorInvalidArgument;
  }

  // Parse headers
  ErrorCode error = parseHeaders(_data.data(), _data.size());
  if (error != kSuccess)
    return error;

  // Determine load address
  // TOS programs typically load at 0x00000000 (with basepage)
  // We'll use a higher address for compatibility
  Address baseAddress = 0x1000;
  loadAddress = baseAddress;

  // Load segments
  Address textAddr, dataAddr, bssAddr;
  error = loadSegments(process, baseAddress, textAddr, dataAddr, bssAddr);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to load segments");
    return error;
  }

  // Process relocations
  if (_hasRelocations) {
    error = processRelocations(process, baseAddress);
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to process relocations");
    }
  }

  // Entry point is at start of text segment
  entryPoint = textAddr;

  DS2LOG(Debug, "TOS loaded at 0x%llx, entry point 0x%llx",
         (unsigned long long)loadAddress.value(),
         (unsigned long long)entryPoint.value());

  return kSuccess;
}

ErrorCode TOSLoader::parseHeaders(void const *data, size_t size) {
  if (size < sizeof(TOSHeader)) {
    DS2LOG(Error, "file too small for TOS header");
    return kErrorInvalidArgument;
  }

  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  // Read TOS header (big-endian)
  memcpy(&_header, ptr, sizeof(TOSHeader));

  // Convert from big-endian
  _header.magic = __builtin_bswap16(_header.magic);
  _header.text_size = __builtin_bswap32(_header.text_size);
  _header.data_size = __builtin_bswap32(_header.data_size);
  _header.bss_size = __builtin_bswap32(_header.bss_size);
  _header.symbol_size = __builtin_bswap32(_header.symbol_size);
  _header.flags = __builtin_bswap32(_header.flags);
  _header.reloc_flag = __builtin_bswap16(_header.reloc_flag);

  // Verify magic
  if (_header.magic != kTOSMagic) {
    DS2LOG(Error, "invalid TOS magic: 0x%04x", _header.magic);
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug,
         "TOS: text=0x%x, data=0x%x, bss=0x%x, symbols=0x%x, "
         "reloc_flag=%d",
         _header.text_size, _header.data_size, _header.bss_size,
         _header.symbol_size, _header.reloc_flag);

  // Check for extended header (MiNT)
  uint32_t headerSize = sizeof(TOSHeader);
  if (headerSize + sizeof(TOSExtendedHeader) <= size) {
    uint32_t extMagic;
    memcpy(&extMagic, ptr + headerSize, sizeof(extMagic));
    extMagic = __builtin_bswap32(extMagic);

    if (extMagic == kTOSExtMagic) {
      _isExtended = true;
      memcpy(&_extHeader, ptr + headerSize, sizeof(TOSExtendedHeader));

      // Convert from big-endian
      _extHeader.magic = __builtin_bswap32(_extHeader.magic);
      _extHeader.info = __builtin_bswap32(_extHeader.info);
      _extHeader.stack_size = __builtin_bswap32(_extHeader.stack_size);
      _extHeader.symbol_format = __builtin_bswap32(_extHeader.symbol_format);
      _extHeader.program_flags = __builtin_bswap32(_extHeader.program_flags);

      DS2LOG(Debug, "MiNT extended header found");
    }
  }

  // Parse relocation table if present
  if (_header.reloc_flag == 0) {
    _hasRelocations = true;
    ErrorCode error = parseRelocationTable();
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to parse relocation table");
    }
  }

  _parsed = true;
  return kSuccess;
}

ErrorCode TOSLoader::loadSegments(ProcessBase *process, Address baseAddress,
                                  Address &textAddr, Address &dataAddr,
                                  Address &bssAddr) {
  if (!_parsed)
    return kErrorInvalidArgument;

  // Calculate segment addresses
  textAddr = baseAddress;
  dataAddr = textAddr + _header.text_size;
  bssAddr = dataAddr + _header.data_size;

  uint32_t headerSize = sizeof(TOSHeader);
  if (_isExtended)
    headerSize += sizeof(TOSExtendedHeader);

  const uint8_t *fileData = _data.data() + headerSize;

  // Load text segment
  if (_header.text_size > 0) {
    ErrorCode error = process->allocateMemory(textAddr, _header.text_size,
                                             0x05); // Read + Execute
    if (error != kSuccess) {
      DS2LOG(Error, "failed to allocate text segment");
      return error;
    }

    error = process->writeMemory(textAddr, fileData, _header.text_size,
                                 nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to write text segment");
      return error;
    }

    fileData += _header.text_size;
  }

  // Load data segment
  if (_header.data_size > 0) {
    ErrorCode error = process->allocateMemory(dataAddr, _header.data_size,
                                             0x03); // Read + Write
    if (error != kSuccess) {
      DS2LOG(Error, "failed to allocate data segment");
      return error;
    }

    error = process->writeMemory(dataAddr, fileData, _header.data_size,
                                 nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to write data segment");
      return error;
    }

    fileData += _header.data_size;
  }

  // Allocate BSS segment (zero-filled)
  if (_header.bss_size > 0) {
    ErrorCode error = process->allocateMemory(bssAddr, _header.bss_size,
                                             0x03); // Read + Write
    if (error != kSuccess) {
      DS2LOG(Error, "failed to allocate BSS segment");
      return error;
    }

    std::vector<uint8_t> zeros(_header.bss_size, 0);
    error = process->writeMemory(bssAddr, zeros.data(), _header.bss_size,
                                 nullptr);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to zero BSS segment");
      return error;
    }
  }

  return kSuccess;
}

ErrorCode TOSLoader::parseRelocationTable() {
  uint32_t headerSize = sizeof(TOSHeader);
  if (_isExtended)
    headerSize += sizeof(TOSExtendedHeader);

  uint32_t dataOffset = headerSize + _header.text_size + _header.data_size +
                        _header.symbol_size;

  if (dataOffset >= _data.size()) {
    DS2LOG(Warning, "no relocation table present");
    return kSuccess;
  }

  const uint8_t *relocData = _data.data() + dataOffset;
  const uint8_t *relocEnd = _data.data() + _data.size();

  // First relocation offset (big-endian)
  if (relocData + 4 > relocEnd)
    return kSuccess;

  uint32_t offset;
  memcpy(&offset, relocData, 4);
  offset = __builtin_bswap32(offset);
  relocData += 4;

  if (offset == 0)
    return kSuccess; // No relocations

  _relocations.push_back({offset});

  // Read subsequent relocations (byte offsets)
  while (relocData < relocEnd) {
    uint8_t delta = *relocData++;

    if (delta == 0)
      break; // End of relocations

    if (delta == 1) {
      // Add 254 to current offset
      offset += 254;
    } else {
      offset += delta;
      _relocations.push_back({offset});
    }
  }

  DS2LOG(Debug, "parsed %zu relocations", _relocations.size());
  return kSuccess;
}

ErrorCode TOSLoader::processRelocations(ProcessBase *process,
                                       Address baseAddress) {
  for (const auto &reloc : _relocations) {
    // Read the value at the relocation offset
    uint32_t value;
    Address relocAddr = baseAddress + reloc.offset;

    ErrorCode error = process->readMemory(relocAddr, &value, sizeof(value),
                                         nullptr);
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to read relocation at 0x%x", reloc.offset);
      continue;
    }

    // Convert from big-endian, add base address, convert back
    value = __builtin_bswap32(value);
    value += baseAddress.value();
    value = __builtin_bswap32(value);

    error = process->writeMemory(relocAddr, &value, sizeof(value), nullptr);
    if (error != kSuccess) {
      DS2LOG(Warning, "failed to write relocation at 0x%x", reloc.offset);
      continue;
    }
  }

  return kSuccess;
}

ErrorCode TOSLoader::getSegments(Address &textAddr, uint32_t &textSize,
                                 Address &dataAddr, uint32_t &dataSize,
                                 Address &bssAddr, uint32_t &bssSize) const {
  if (!_parsed)
    return kErrorInvalidArgument;

  textSize = _header.text_size;
  dataSize = _header.data_size;
  bssSize = _header.bss_size;

  // These would be filled in during load
  return kSuccess;
}

} // namespace Atari
} // namespace Target
} // namespace ds2
