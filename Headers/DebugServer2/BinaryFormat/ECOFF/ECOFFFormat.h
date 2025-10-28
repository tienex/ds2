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

#include "DebugServer2/Types.h"
#include <cstdint>
#include <vector>

namespace ds2 {
namespace BinaryFormat {
namespace ECOFF {

// ECOFF (Extended Common Object File Format)
// Used by various Unix systems on MIPS, Alpha, and other RISC architectures
// Base format for OSF/ROSE

// ECOFF File Header
struct ECOFFFileHeader {
  uint16_t f_magic;       // Magic number
  uint16_t f_nscns;       // Number of sections
  uint32_t f_timdat;      // Time & date stamp
  uint32_t f_symptr;      // File pointer to symbolic header
  uint32_t f_nsyms;       // Size of symbolic header
  uint16_t f_opthdr;      // Size of optional header
  uint16_t f_flags;       // Flags
};

// ECOFF Magic numbers
enum ECOFFMagic {
  ECOFF_MAGIC_MIPSEB = 0x0160,     // MIPS big-endian
  ECOFF_MAGIC_MIPSEL = 0x0162,     // MIPS little-endian
  ECOFF_MAGIC_MIPS2EB = 0x0163,    // MIPS-II big-endian
  ECOFF_MAGIC_MIPS2EL = 0x0166,    // MIPS-II little-endian
  ECOFF_MAGIC_MIPS3EB = 0x0140,    // MIPS-III big-endian
  ECOFF_MAGIC_MIPS3EL = 0x0142,    // MIPS-III little-endian
  ECOFF_MAGIC_ALPHA = 0x0183,      // DEC Alpha
  ECOFF_MAGIC_ALPHA_EXP = 0x0188,  // Alpha experimental
};

// ECOFF flags
enum ECOFFFlags {
  F_RELFLG = 0x0001,    // Relocation info stripped
  F_EXEC = 0x0002,      // File is executable
  F_LNNO = 0x0004,      // Line numbers stripped
  F_LSYMS = 0x0008,     // Local symbols stripped
  F_MINMAL = 0x0010,    // Minimal object file
  F_UPDATE = 0x0020,    // Update file
  F_SWABD = 0x0040,     // Swabbed bytes
  F_AR16WR = 0x0080,    // 16-bit reversed
  F_AR32WR = 0x0100,    // 32-bit reversed
  F_AR32W = 0x0200,     // 32-bit words
  F_PATCH = 0x0400,     // Patch list
  F_NODF = 0x0400,      // No decision functions
};

// ECOFF Optional Header (a.out header)
struct ECOFFAoutHeader {
  uint16_t magic;         // Magic number
  uint16_t vstamp;        // Version stamp
  uint32_t tsize;         // Text size in bytes
  uint32_t dsize;         // Initialized data size
  uint32_t bsize;         // Uninitialized data size
  uint32_t entry;         // Entry point
  uint32_t text_start;    // Base of text used for this file
  uint32_t data_start;    // Base of data used for this file
  uint32_t bss_start;     // Base of bss used for this file
  uint32_t gprmask;       // General purpose register mask
  uint32_t cprmask[4];    // Co-processor register masks
  uint32_t gp_value;      // GP register value
};

// ECOFF Section Header
struct ECOFFSectionHeader {
  char s_name[8];         // Section name
  uint32_t s_paddr;       // Physical address
  uint32_t s_vaddr;       // Virtual address
  uint32_t s_size;        // Section size
  uint32_t s_scnptr;      // File pointer to raw data
  uint32_t s_relptr;      // File pointer to relocation
  uint32_t s_lnnoptr;     // File pointer to line numbers
  uint16_t s_nreloc;      // Number of relocation entries
  uint16_t s_nlnno;       // Number of line number entries
  uint32_t s_flags;       // Flags
};

// ECOFF section types
enum ECOFFSectionType {
  STYP_REG = 0x00,        // Regular section
  STYP_TEXT = 0x20,       // Text section
  STYP_DATA = 0x40,       // Data section
  STYP_BSS = 0x80,        // BSS section
  STYP_RDATA = 0x100,     // Read-only data
  STYP_SDATA = 0x200,     // Small data (GP-relative)
  STYP_SBSS = 0x400,      // Small BSS
  STYP_LIT8 = 0x08000000, // 8-byte literals
  STYP_LIT4 = 0x10000000, // 4-byte literals
};

// ECOFF Symbolic Header
struct ECOFFSymbolicHeader {
  int16_t magic;          // Magic number
  int16_t vstamp;         // Version stamp
  int32_t ilineMax;       // Number of line number entries
  int32_t cbLine;         // Number of bytes for line number entries
  int32_t cbLineOffset;   // Offset to start of line number entries
  int32_t idnMax;         // Max index into dense number table
  int32_t cbDnOffset;     // Offset to start dense number table
  int32_t ipdMax;         // Number of procedures
  int32_t cbPdOffset;     // Offset to procedure descriptor table
  int32_t isymMax;        // Number of local symbols
  int32_t cbSymOffset;    // Offset to start of local symbols
  int32_t ioptMax;        // Max index into optimization symbol entries
  int32_t cbOptOffset;    // Offset to optimization symbol entries
  int32_t iauxMax;        // Number of auxiliary symbol entries
  int32_t cbAuxOffset;    // Offset to start of auxiliary symbol entries
  int32_t issMax;         // Max index into local strings
  int32_t cbSsOffset;     // Offset to start of local strings
  int32_t issExtMax;      // Max index into external strings
  int32_t cbSsExtOffset;  // Offset to start of external strings
  int32_t ifdMax;         // Number of file descriptor entries
  int32_t cbFdOffset;     // Offset to file descriptor table
  int32_t crfd;           // Number of relative file descriptor entries
  int32_t cbRfdOffset;    // Offset to relative file descriptor table
  int32_t iextMax;        // Max index into external symbols
  int32_t cbExtOffset;    // Offset to start of external symbol entries
};

// ECOFF Symbol Entry
struct ECOFFSymbol {
  uint32_t iss;           // Index into string space
  uint32_t value;         // Value of symbol
  uint32_t st : 6;        // Symbol type
  uint32_t sc : 5;        // Storage class
  uint32_t reserved : 1;  // Reserved
  uint32_t index : 20;    // Index into sym/aux table
};

// Symbol types
enum ECOFFSymbolType {
  stNil = 0,      // Nil
  stGlobal = 1,   // External symbol
  stStatic = 2,   // Static
  stParam = 3,    // Procedure argument
  stLocal = 4,    // Local variable
  stLabel = 5,    // Label
  stProc = 6,     // Procedure
  stBlock = 7,    // Beginning of block
  stEnd = 8,      // End of block/procedure
  stMember = 9,   // Member of struct/union/enum
  stTypedef = 10, // Type definition
  stFile = 11,    // File name
  stRegReloc = 12,// Register relocation
  stForward = 13, // Forward reference
  stStaticProc = 14, // Static procedure
  stConstant = 15,// Constant
};

// Storage classes
enum ECOFFStorageClass {
  scNil = 0,       // Nil
  scText = 1,      // Text symbol
  scData = 2,      // Data symbol
  scBss = 3,       // BSS symbol
  scRegister = 4,  // Register variable
  scAbs = 5,       // Absolute symbol
  scUndefined = 6, // Undefined external
  scCdbLocal = 7,  // Variable's value in CDB
  scBits = 8,      // Bit field
  scCdbSystem = 9, // Variable's value in CDB (system)
  scRegImage = 10, // Register value saved on stack
  scInfo = 11,     // Symbol contains debugger info
  scUserStruct = 12, // Address in struct user for current process
  scSData = 13,    // Small data (GP-relative)
  scSBss = 14,     // Small BSS (GP-relative)
  scRData = 15,    // Read-only data
  scVar = 16,      // Var parameter by reference
  scCommon = 17,   // Common variable
  scSCommon = 18,  // Small common
  scVarRegister = 19, // Var parameter in register
  scVariant = 20,  // Variant record
  scSUndefined = 21, // Small undefined external
  scInit = 22,     // .init section
};

// Helper class for ECOFF format parsing
class ECOFFParser {
public:
  static bool isECOFF(const uint8_t *data, size_t size);
  static bool parseFileHeader(const uint8_t *data, size_t size,
                             ECOFFFileHeader &header);
  static bool parseAoutHeader(const uint8_t *data, size_t size,
                             const ECOFFFileHeader &fhdr,
                             ECOFFAoutHeader &aout);
  static bool parseSectionHeaders(const uint8_t *data, size_t size,
                                 const ECOFFFileHeader &fhdr,
                                 std::vector<ECOFFSectionHeader> &sections);
  static bool parseSymbolicHeader(const uint8_t *data, size_t size,
                                 const ECOFFFileHeader &fhdr,
                                 ECOFFSymbolicHeader &symhdr);
  static uint32_t getEntryPoint(const ECOFFAoutHeader &aout);
  static bool isMIPS(const ECOFFFileHeader &header);
  static bool isAlpha(const ECOFFFileHeader &header);
  static bool isBigEndian(const ECOFFFileHeader &header);
};

} // namespace ECOFF
} // namespace BinaryFormat
} // namespace ds2
