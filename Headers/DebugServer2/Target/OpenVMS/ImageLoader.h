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
namespace OpenVMS {

// OpenVMS Executable Image Format
// Used on VAX, Alpha, IA-64, and x86-64 VMS systems
// Format includes image header, image section descriptors (ISDs), and fixup data

// Image Header Block (IHD)
struct ImageHeader {
  uint16_t size;              // Size of this block
  uint16_t version;           // Version number
  uint32_t hdrflags;          // Header flags
  uint32_t iafva;             // Image activator virtual address
  uint32_t lnkflags;          // Linker flags
  uint32_t imgidoff;          // Image identification offset
  uint32_t patchoff;          // Patch text offset
  uint32_t imgbid[4];         // Image build ID
  uint32_t lnkid[2];          // Linker ID
  uint32_t symvva;            // Symbol vector virtual address
  uint32_t symvblks;          // Symbol vector blocks
  uint32_t majid;             // Major ID
  uint32_t minorid;           // Minor ID
  uint32_t imgnam[40];        // Image name
};

// Image Section Descriptor (ISD)
struct ImageSectionDescriptor {
  uint16_t size;              // Size of this ISD
  uint16_t type;              // ISD type
  uint32_t flags;             // Section flags
  uint32_t vbn;               // Virtual block number
  uint32_t pagcnt;            // Page count
  uint32_t vaddr;             // Virtual address (VAX) or RVA
  uint64_t vaddr64;           // Virtual address (Alpha/IA64/x86-64)
};

// ISD Types
enum ISDType {
  kISDTypeNormal = 0,         // Normal section
  kISDTypeFixup = 1,          // Fixup section
  kISDTypeGlobalSymbol = 2,   // Global symbol table
  kISDTypeDebug = 3,          // Debug information
  kISDTypeDemandZero = 4,     // Demand-zero section
};

// ISD Flags
enum ISDFlags {
  kISDFlagWritable = 0x0001,  // Writable
  kISDFlagExecutable = 0x0002,// Executable
  kISDFlagResident = 0x0004,  // Memory resident
  kISDFlagCopyOnRef = 0x0008, // Copy on reference
  kISDFlagVector = 0x0010,    // Contains vectors
  kISDFlagProtected = 0x0020, // Protected
  kISDFlagInitialized = 0x0040,// Initialized data
};

// Fixup types
enum FixupType {
  kFixupAbsolute = 0,         // Absolute address
  kFixupSelfRel = 1,          // Self-relative
  kFixupPICBase = 2,          // Position-independent code base
  kFixupLinkPair = 3,         // Linkage pair
  kFixupQuadWord = 4,         // Quadword fixup (Alpha)
};

class ImageLoader {
public:
  ImageLoader();
  ~ImageLoader();

public:
  // Load OpenVMS executable image
  ErrorCode load(ProcessBase *process, std::string const &path,
                 Address &loadAddress, Address &entryPoint);

  // Parse image headers
  ErrorCode parseHeaders(void const *data, size_t size);

  // Get image sections
  ErrorCode getSections(std::vector<ImageSectionDescriptor> &sections) const;

  // Check architecture
  bool isVAX() const { return _isVAX; }
  bool isAlpha() const { return _isAlpha; }
  bool isIA64() const { return _isIA64; }
  bool isx64() const { return _isx64; }

private:
  ImageHeader _header;
  std::vector<ImageSectionDescriptor> _sections;
  std::vector<uint8_t> _data;
  bool _parsed;
  bool _isVAX;
  bool _isAlpha;
  bool _isIA64;
  bool _isx64;

private:
  ErrorCode loadSection(ProcessBase *process, size_t sectionIndex,
                       Address baseAddress, Address &sectionAddress);
  ErrorCode processFixups(ProcessBase *process, Address baseAddress);
  
  // Read string from image
  std::string readString(uint32_t offset) const;
};

} // namespace OpenVMS
} // namespace Target
} // namespace ds2
