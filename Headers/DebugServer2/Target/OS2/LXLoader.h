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
namespace OS2 {

// OS/2 LX (Linear eXecutable) format
// Used in OS/2 2.0 and later, including OS/2 Warp
// Also used in some DOS extenders

// LX Header (follows DOS stub)
struct LXHeader {
  uint16_t magic;             // 'LX' (0x584C) or 'LE' (0x454C)
  uint8_t byteOrder;          // Byte ordering (0=little endian)
  uint8_t wordOrder;          // Word ordering
  uint32_t formatLevel;       // Format level
  uint16_t cpuType;           // CPU type (1=286, 2=386, 3=486, 4=Pentium)
  uint16_t osType;            // OS type (1=OS/2, 2=Windows, 3=DOS 4.x, 4=Windows 386)
  uint32_t moduleVersion;     // Module version
  uint32_t moduleFlags;       // Module type flags
  uint32_t modulePages;       // Number of module pages
  uint32_t eipObject;         // Object number for EIP
  uint32_t eip;               // Starting EIP
  uint32_t espObject;         // Object number for ESP
  uint32_t esp;               // Starting ESP
  uint32_t pageSize;          // Memory page size
  uint32_t pageShift;         // Page alignment shift (LE only)
  uint32_t fixupSize;         // Fixup section size
  uint32_t fixupChecksum;     // Fixup section checksum
  uint32_t loaderSize;        // Loader section size
  uint32_t loaderChecksum;    // Loader section checksum
  uint32_t objectTableOffset; // Object table offset
  uint32_t objectCount;       // Number of objects
  uint32_t objectPageTableOffset; // Object page table offset
  uint32_t objectIterPagesOffset; // Object iterated pages offset
  uint32_t resourceTableOffset;   // Resource table offset
  uint32_t resourceCount;     // Number of resources
  uint32_t residentNameTableOffset; // Resident name table offset
  uint32_t entryTableOffset;  // Entry table offset
  uint32_t moduleDirectivesOffset; // Module directives offset
  uint32_t moduleDirectivesCount;  // Number of directives
  uint32_t fixupPageTableOffset;   // Fixup page table offset
  uint32_t fixupRecordTableOffset; // Fixup record table offset
  uint32_t importModuleTableOffset; // Import module name table offset
  uint32_t importModuleCount; // Number of import modules
  uint32_t importProcTableOffset;  // Import procedure name table offset
  uint32_t perPageChecksumOffset;  // Per-page checksum offset
  uint32_t dataPages;         // Number of preload pages (LE only)
  uint32_t preloadPages;      // Number of preload pages
  uint32_t nonResidentNameTableOffset; // Non-resident name table offset
  uint32_t nonResidentNameTableSize;   // Non-resident name table size
  uint32_t nonResidentNameChecksum;    // Non-resident name checksum
  uint32_t autoDataObject;    // Auto data segment object number
  uint32_t debugInfoOffset;   // Debug info offset
  uint32_t debugInfoSize;     // Debug info size
  uint32_t instancePreloadPages;      // Instance preload pages
  uint32_t instanceDemandPages;       // Instance demand pages
  uint32_t heapSize;          // Initial heap size
  uint32_t stackSize;         // Initial stack size
};

// Object table entry
struct LXObject {
  uint32_t virtualSize;       // Virtual segment size
  uint32_t relocBaseAddr;     // Relocation base address
  uint32_t objectFlags;       // Object flags
  uint32_t pageTableIndex;    // Page table index
  uint32_t pageTableEntries;  // Number of page table entries
  uint32_t reserved;          // Reserved
};

// Object flags
enum LXObjectFlags {
  kLXObjectReadable = 0x0001,
  kLXObjectWritable = 0x0002,
  kLXObjectExecutable = 0x0004,
  kLXObjectResource = 0x0008,
  kLXObjectDiscardable = 0x0010,
  kLXObjectShared = 0x0020,
  kLXObjectPreload = 0x0040,
  kLXObjectInvalid = 0x0080,
  kLXObjectZeroFilled = 0x0100,
  kLXObjectResident = 0x0200,
  kLXObjectLongLocRel = 0x0400,
  kLXObject16_16Alias = 0x1000,
  kLXObjectBigDefault = 0x2000,
  kLXObjectConforming = 0x4000,
  kLXObjectIOPrivilege = 0x8000
};

// Page table entry
struct LXPageTableEntry {
  uint32_t pageDataOffset;    // High byte: flags, low 3 bytes: offset
  uint16_t dataSize;          // Data size in file
  uint16_t flags;             // Page flags
};

class LXLoader {
public:
  LXLoader();
  ~LXLoader();

public:
  // Load LX/LE executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse LX/LE headers
  ErrorCode parseHeaders(void const *data, size_t size);

  // Get objects (segments)
  ErrorCode getObjects(std::vector<Section> &objects) const;

  // Get imported modules
  ErrorCode getImportedModules(std::vector<std::string> &modules) const;

  // Determine if LE or LX format
  bool isLE() const { return _isLE; }
  bool isLX() const { return !_isLE; }

private:
  LXHeader _header;
  std::vector<LXObject> _objects;
  std::vector<uint8_t> _data;
  bool _parsed;
  bool _isLE;  // true for LE, false for LX

private:
  ErrorCode loadObject(ProcessBase *process, size_t objectIndex,
                      Address baseAddress, Address &objectAddress);
  ErrorCode processFixups(ProcessBase *process, Address baseAddress);
  ErrorCode loadPage(ProcessBase *process, uint32_t pageIndex,
                    Address address);
};

} // namespace OS2
} // namespace Target
} // namespace ds2
