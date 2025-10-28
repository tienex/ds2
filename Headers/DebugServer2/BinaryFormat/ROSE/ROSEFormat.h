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
namespace ROSE {

// OSF/ROSE (RISC Object System Environment) Executable Format
// Based on Mach-O format from NeXT/Apple
// Used by OSF/1 ADE (Application Development Environment)
// Developed by Open Software Foundation for RISC workstations

// ROSE File Header (based on Mach-O)
struct ROSEHeader {
  uint32_t magic;           // Magic number
  uint32_t cputype;         // CPU type
  uint32_t cpusubtype;      // CPU subtype
  uint32_t filetype;        // File type
  uint32_t ncmds;           // Number of load commands
  uint32_t sizeofcmds;      // Size of load commands
  uint32_t flags;           // Flags
};

// ROSE magic numbers
enum ROSEMagic {
  ROSE_MAGIC = 0xFEEDFACE,         // Native byte order
  ROSE_CIGAM = 0xCEFAEDFE,         // Reverse byte order
  ROSE_MAGIC_64 = 0xFEEDFACF,      // 64-bit native
  ROSE_CIGAM_64 = 0xCFFAEDFE,      // 64-bit reverse
};

// CPU types (OSF/ROSE supported architectures)
enum ROSECPUType {
  ROSE_CPU_TYPE_ANY = -1,
  ROSE_CPU_TYPE_VAX = 1,
  ROSE_CPU_TYPE_ROMP = 2,           // IBM RT PC
  ROSE_CPU_TYPE_NS32032 = 4,        // National Semiconductor 32032
  ROSE_CPU_TYPE_NS32332 = 5,        // National Semiconductor 32332
  ROSE_CPU_TYPE_MC680x0 = 6,        // Motorola 68K
  ROSE_CPU_TYPE_I386 = 7,           // Intel x86
  ROSE_CPU_TYPE_MIPS = 8,           // MIPS
  ROSE_CPU_TYPE_NS32532 = 9,        // National Semiconductor 32532
  ROSE_CPU_TYPE_HPPA = 11,          // HP PA-RISC
  ROSE_CPU_TYPE_ARM = 12,           // ARM
  ROSE_CPU_TYPE_MC88000 = 13,       // Motorola 88K
  ROSE_CPU_TYPE_SPARC = 14,         // SPARC
  ROSE_CPU_TYPE_I860 = 15,          // Intel i860
  ROSE_CPU_TYPE_ALPHA = 16,         // DEC Alpha
  ROSE_CPU_TYPE_RS6000 = 17,        // IBM RS/6000 (POWER)
  ROSE_CPU_TYPE_POWERPC = 18,       // PowerPC
};

// File types
enum ROSEFileType {
  ROSE_MH_OBJECT = 0x1,             // Relocatable object file
  ROSE_MH_EXECUTE = 0x2,            // Demand paged executable
  ROSE_MH_FVMLIB = 0x3,             // Fixed VM shared library
  ROSE_MH_CORE = 0x4,               // Core file
  ROSE_MH_PRELOAD = 0x5,            // Preloaded executable
  ROSE_MH_DYLIB = 0x6,              // Dynamically bound shared library
  ROSE_MH_DYLINKER = 0x7,           // Dynamic link editor
  ROSE_MH_BUNDLE = 0x8,             // Dynamically bound bundle
};

// Flags
enum ROSEFlags {
  ROSE_MH_NOUNDEFS = 0x1,           // No undefined references
  ROSE_MH_INCRLINK = 0x2,           // Output of incremental link
  ROSE_MH_DYLDLINK = 0x4,           // Input for dynamic linker
  ROSE_MH_BINDATLOAD = 0x8,         // Undefined refs bound dynamically
  ROSE_MH_PREBOUND = 0x10,          // File has prebinding info
  ROSE_MH_SPLIT_SEGS = 0x20,        // Read-only and read-write segs split
  ROSE_MH_LAZY_INIT = 0x40,         // Shared lib init routine run lazily
  ROSE_MH_TWOLEVEL = 0x80,          // Two-level namespace bindings
  ROSE_MH_FORCE_FLAT = 0x100,       // Force flat namespace bindings
};

// Load Command Header
struct ROSELoadCommand {
  uint32_t cmd;             // Command type
  uint32_t cmdsize;         // Size of command including data
};

// Load command types
enum ROSELoadCommandType {
  ROSE_LC_SEGMENT = 0x1,            // Segment to be mapped
  ROSE_LC_SYMTAB = 0x2,             // Symbol table info
  ROSE_LC_SYMSEG = 0x3,             // Symbol segment (obsolete)
  ROSE_LC_THREAD = 0x4,             // Thread state
  ROSE_LC_UNIXTHREAD = 0x5,         // Unix thread (includes stack)
  ROSE_LC_LOADFVMLIB = 0x6,         // Load fixed VM library
  ROSE_LC_IDFVMLIB = 0x7,           // Fixed VM library ID
  ROSE_LC_IDENT = 0x8,              // Object identification (obsolete)
  ROSE_LC_FVMFILE = 0x9,            // Fixed VM file inclusion
  ROSE_LC_PREPAGE = 0xa,            // Prepage command (internal)
  ROSE_LC_DYSYMTAB = 0xb,           // Dynamic symbol table info
  ROSE_LC_LOAD_DYLIB = 0xc,         // Load dynamic shared library
  ROSE_LC_ID_DYLIB = 0xd,           // Dynamic shared library ID
  ROSE_LC_LOAD_DYLINKER = 0xe,      // Load dynamic linker
  ROSE_LC_ID_DYLINKER = 0xf,        // Dynamic linker ID
  ROSE_LC_PREBOUND_DYLIB = 0x10,    // Prebound modules
};

// Segment Command (32-bit)
struct ROSESegmentCommand {
  uint32_t cmd;             // LC_SEGMENT
  uint32_t cmdsize;         // Size of segment command
  char segname[16];         // Segment name
  uint32_t vmaddr;          // Virtual memory address
  uint32_t vmsize;          // Virtual memory size
  uint32_t fileoff;         // File offset
  uint32_t filesize;        // File size
  uint32_t maxprot;         // Maximum VM protection
  uint32_t initprot;        // Initial VM protection
  uint32_t nsects;          // Number of sections
  uint32_t flags;           // Flags
};

// Segment Command (64-bit)
struct ROSESegmentCommand64 {
  uint32_t cmd;             // LC_SEGMENT_64
  uint32_t cmdsize;         // Size of segment command
  char segname[16];         // Segment name
  uint64_t vmaddr;          // Virtual memory address
  uint64_t vmsize;          // Virtual memory size
  uint64_t fileoff;         // File offset
  uint64_t filesize;        // File size
  uint32_t maxprot;         // Maximum VM protection
  uint32_t initprot;        // Initial VM protection
  uint32_t nsects;          // Number of sections
  uint32_t flags;           // Flags
};

// Section (32-bit)
struct ROSESection {
  char sectname[16];        // Section name
  char segname[16];         // Segment this section is in
  uint32_t addr;            // Address of this section
  uint32_t size;            // Size in bytes
  uint32_t offset;          // File offset
  uint32_t align;           // Section alignment (power of 2)
  uint32_t reloff;          // File offset of relocation entries
  uint32_t nreloc;          // Number of relocation entries
  uint32_t flags;           // Flags
  uint32_t reserved1;       // Reserved
  uint32_t reserved2;       // Reserved
};

// Section (64-bit)
struct ROSESection64 {
  char sectname[16];        // Section name
  char segname[16];         // Segment this section is in
  uint64_t addr;            // Address of this section
  uint64_t size;            // Size in bytes
  uint32_t offset;          // File offset
  uint32_t align;           // Section alignment (power of 2)
  uint32_t reloff;          // File offset of relocation entries
  uint32_t nreloc;          // Number of relocation entries
  uint32_t flags;           // Flags
  uint32_t reserved1;       // Reserved
  uint32_t reserved2;       // Reserved
  uint32_t reserved3;       // Reserved
};

// Symbol Table Command
struct ROSESymtabCommand {
  uint32_t cmd;             // LC_SYMTAB
  uint32_t cmdsize;         // Size of command
  uint32_t symoff;          // Symbol table offset
  uint32_t nsyms;           // Number of symbol entries
  uint32_t stroff;          // String table offset
  uint32_t strsize;         // String table size
};

// Dynamic Symbol Table Command
struct ROSEDysymtabCommand {
  uint32_t cmd;             // LC_DYSYMTAB
  uint32_t cmdsize;         // Size of command
  uint32_t ilocalsym;       // Index of first local symbol
  uint32_t nlocalsym;       // Number of local symbols
  uint32_t iextdefsym;      // Index of first externally defined symbol
  uint32_t nextdefsym;      // Number of externally defined symbols
  uint32_t iundefsym;       // Index of first undefined symbol
  uint32_t nundefsym;       // Number of undefined symbols
  uint32_t tocoff;          // File offset to table of contents
  uint32_t ntoc;            // Number of entries in table of contents
  uint32_t modtaboff;       // File offset to module table
  uint32_t nmodtab;         // Number of entries in module table
  uint32_t extrefsymoff;    // File offset to referenced symbol table
  uint32_t nextrefsyms;     // Number of referenced symbol entries
  uint32_t indirectsymoff;  // File offset to indirect symbol table
  uint32_t nindirectsyms;   // Number of indirect symbol entries
  uint32_t extreloff;       // File offset to external relocation entries
  uint32_t nextrel;         // Number of external relocation entries
  uint32_t locreloff;       // File offset to local relocation entries
  uint32_t nlocrel;         // Number of local relocation entries
};

// Symbol Entry (nlist structure)
struct ROSENList {
  uint32_t n_strx;          // String table index
  uint8_t n_type;           // Type flag
  uint8_t n_sect;           // Section number
  int16_t n_desc;           // Description field
  uint32_t n_value;         // Value
};

// Symbol Entry 64-bit
struct ROSENList64 {
  uint32_t n_strx;          // String table index
  uint8_t n_type;           // Type flag
  uint8_t n_sect;           // Section number
  uint16_t n_desc;          // Description field
  uint64_t n_value;         // Value
};

// Protection flags
enum ROSEVMProtection {
  ROSE_VM_PROT_NONE = 0x00,
  ROSE_VM_PROT_READ = 0x01,
  ROSE_VM_PROT_WRITE = 0x02,
  ROSE_VM_PROT_EXECUTE = 0x04,
};

// Helper class for ROSE format parsing
class ROSEParser {
public:
  static bool isROSE(const uint8_t *data, size_t size);
  static bool parseHeader(const uint8_t *data, size_t size,
                         ROSEHeader &header);
  static bool is64Bit(const ROSEHeader &header);
  static bool isByteSwapped(const ROSEHeader &header);
  static bool parseLoadCommands(const uint8_t *data, size_t size,
                               const ROSEHeader &header,
                               std::vector<ROSELoadCommand> &commands);
  static bool parseSegments(const uint8_t *data, size_t size,
                           const ROSEHeader &header,
                           std::vector<ROSESegmentCommand> &segments);
  static bool parseSymbolTable(const uint8_t *data, size_t size,
                              const ROSEHeader &header,
                              ROSESymtabCommand &symtab);
  static uint64_t getEntryPoint(const uint8_t *data, size_t size,
                               const ROSEHeader &header);
  static std::string getCPUTypeName(uint32_t cputype);
  static std::string getFileTypeName(uint32_t filetype);
};

} // namespace ROSE
} // namespace BinaryFormat
} // namespace ds2
