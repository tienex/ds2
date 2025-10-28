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
#include <map>
#include <string>
#include <vector>

namespace ds2 {
namespace Target {
namespace MacOS {

// Classic Mac OS Resource Manager Executable Format
// Used on Classic Mac OS (System 1-9) for 68k and PowerPC
// Format includes resource fork with CODE resources and jump table

// Resource File Header (Resource Fork)
struct ResourceHeader {
  uint32_t dataOffset;       // Offset to resource data
  uint32_t mapOffset;        // Offset to resource map
  uint32_t dataLength;       // Length of resource data
  uint32_t mapLength;        // Length of resource map
};

// Resource Map Header
struct ResourceMapHeader {
  uint16_t resourceForkAttrs; // Resource fork attributes
  uint32_t typeListOffset;    // Offset to type list
  uint32_t nameListOffset;    // Offset to name list
  uint16_t numTypesMinusOne;  // Number of types - 1
};

// Resource Type Entry
struct ResourceType {
  uint32_t type;             // Resource type (4 chars, e.g., 'CODE')
  uint16_t numResourcesMinusOne; // Number of resources - 1
  uint16_t refListOffset;    // Offset to reference list
};

// Resource Reference
struct ResourceReference {
  uint16_t resourceID;       // Resource ID
  uint16_t nameOffset;       // Offset to name (-1 if none)
  uint8_t attributes;        // Resource attributes
  uint8_t dataOffsetHi;      // High byte of data offset
  uint16_t dataOffsetLo;     // Low word of data offset
  uint32_t reserved;         // Reserved (handle)
};

// CODE Resource Types
enum CodeResourceType {
  kCodeResource = 0x434F4445,      // 'CODE' - 68k code
  kPEFResource = 0x63667267,       // 'cfrg' - Code Fragment (PowerPC)
  kSizeResource = 0x53495A45,      // 'SIZE' - Memory size
};

// CODE Resource #0 (Jump Table Header)
struct JumpTableHeader {
  uint16_t aboveA5Size;      // Size above A5
  uint32_t appParams;        // Application parameters
  uint32_t jumpTableSize;    // Jump table size
  uint32_t jumpTableOffset;  // Jump table offset
};

// Jump Table Entry
struct JumpTableEntry {
  uint16_t offset;           // Offset into CODE segment
  uint16_t codeID;           // CODE resource ID (0x3F for A-trap)
  uint16_t pushAddr;         // 0x3F for A-trap entries
};

// CODE Resource Header (for CODE 1, CODE 2, etc.)
struct CodeResourceHeader {
  uint16_t entryOffset;      // Offset to entry point
  uint16_t numJTEntries;     // Number of jump table entries
};

// SIZE Resource (Memory Requirements)
struct SizeResource {
  uint16_t flags;            // Memory flags
  uint32_t preferredSize;    // Preferred memory size
  uint32_t minimumSize;      // Minimum memory size
};

// SIZE Resource Flags
enum SizeFlags {
  kSizeFlagDeskAccessory = 0x0001,    // Desk accessory
  kSizeFlagMultiFinderAware = 0x0002, // MultiFinder aware
  kSizeFlagBackgroundNull = 0x0004,   // Background null events
  kSizeFlagGetFrontClicks = 0x0008,   // Get front clicks
  kSizeFlagAcceptSuspend = 0x0010,    // Accept suspend/resume
  kSizeFlagStationery = 0x0020,       // Stationery aware
  kSizeFlagUseTextEdit = 0x0040,      // Use TextEdit
  kSizeFlagOnlyBackground = 0x0080,   // Only run in background
  kSizeFlagGetAppDied = 0x0100,       // Get app died message
  kSizeFlagHighLevel = 0x0200,        // High-level event aware
  kSizeFlagLocalHLEvents = 0x0400,    // Local high-level events
  kSizeFlagStationeryAware = 0x0800,  // Stationery aware
  kSizeFlagUse32BitMode = 0x1000,     // Use 32-bit mode
};

// PEF (Preferred Executable Format) for PowerPC
struct PEFContainerHeader {
  uint32_t tag1;             // 'Joy!' magic number
  uint32_t tag2;             // 'peff' magic number
  uint32_t architecture;     // Architecture type
  uint32_t formatVersion;    // Format version
  uint32_t dateTimeStamp;    // Date/time stamp
  uint32_t oldDefVersion;    // Old definition version
  uint32_t oldImpVersion;    // Old implementation version
  uint32_t currentVersion;   // Current version
  uint16_t sectionCount;     // Number of sections
  uint16_t instSectionCount; // Number of instantiated sections
  uint32_t reservedA;        // Reserved
};

// PEF Section Header
struct PEFSectionHeader {
  int32_t nameOffset;        // Section name offset
  uint32_t defaultAddress;   // Default address
  uint32_t totalLength;      // Total length
  uint32_t unpackedLength;   // Unpacked length
  uint32_t containerLength;  // Container length
  uint32_t containerOffset;  // Container offset
  uint8_t sectionKind;       // Section kind
  uint8_t shareKind;         // Share kind
  uint8_t alignment;         // Alignment
  uint8_t reserved;          // Reserved
};

// Resource Entry
struct Resource {
  uint32_t type;             // Resource type
  uint16_t id;               // Resource ID
  std::string name;          // Resource name
  std::vector<uint8_t> data; // Resource data
  uint8_t attributes;        // Attributes
};

class ResourceLoader {
public:
  ResourceLoader();
  ~ResourceLoader();

public:
  // Load Classic Mac OS executable
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Load from resource fork data
  ErrorCode loadResourceFork(void const *data, size_t size);

  // Get resources by type
  ErrorCode getResources(uint32_t type, std::vector<Resource> &resources) const;

  // Get specific resource
  ErrorCode getResource(uint32_t type, uint16_t id, Resource &resource) const;

  // Check executable type
  bool is68k() const { return _is68k; }
  bool isPowerPC() const { return _isPowerPC; }

  // Get memory requirements
  ErrorCode getMemoryRequirements(uint32_t &preferredSize,
                                 uint32_t &minimumSize) const;

private:
  std::map<uint32_t, std::map<uint16_t, Resource>> _resources;
  std::vector<uint8_t> _data;
  bool _parsed;
  bool _is68k;
  bool _isPowerPC;
  JumpTableHeader _jumpTable;

private:
  ErrorCode parseResourceFork(void const *data, size_t size);
  ErrorCode loadCodeResources(ProcessBase *process, Address baseAddress,
                             Address &entryPoint);
  ErrorCode loadPEFContainer(ProcessBase *process, Address baseAddress,
                            Address &entryPoint);
  ErrorCode setupJumpTable(ProcessBase *process, Address a5);

  // Read big-endian values
  uint16_t readBE16(const uint8_t *&ptr);
  uint32_t readBE32(const uint8_t *&ptr);

  // Read Pascal string
  std::string readPString(const uint8_t *ptr);
};

} // namespace MacOS
} // namespace Target
} // namespace ds2
