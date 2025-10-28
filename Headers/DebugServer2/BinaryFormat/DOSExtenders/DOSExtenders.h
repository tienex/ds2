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
#include "DebugServer2/BinaryFormat/MZ/MZFormat.h"
#include <cstdint>
#include <vector>
#include <string>

namespace ds2 {
namespace BinaryFormat {
namespace DOSExtenders {

// Comprehensive DOS Extenders Support
// DOS extenders allow 16-bit DOS to run 32-bit protected mode applications

// ============================================================================
// DOS/4G, DOS/4GW, DOS/32 Advanced - Rational Systems, Tenberry Software
// ============================================================================

// LE (Linear Executable) Header - used by DOS/4GW, DOS/32A
struct LEHeader {
  uint16_t signature;         // 'LE' (0x454C) or 'LX' (0x584C)
  uint8_t byte_order;         // 0=little-endian, 1=big-endian
  uint8_t word_order;         // Word order
  uint32_t format_level;      // Executable format level
  uint16_t cpu_type;          // CPU type (1=286, 2=386, 3=486, 4=586)
  uint16_t os_type;           // Target OS (1=OS/2, 2=Windows, 3=DOS/4, 4=Windows 386)
  uint32_t module_version;    // Module version
  uint32_t module_flags;      // Module type flags
  uint32_t num_pages;         // Number of memory pages
  uint32_t eip_object;        // Object number for EIP
  uint32_t eip;               // Initial EIP value
  uint32_t esp_object;        // Object number for ESP
  uint32_t esp;               // Initial ESP value
  uint32_t page_size;         // Memory page size
  uint32_t page_offset_shift; // Page offset shift (LX)
  uint32_t fixup_size;        // Fixup section size
  uint32_t fixup_checksum;    // Fixup section checksum
  uint32_t loader_size;       // Loader section size
  uint32_t loader_checksum;   // Loader section checksum
  uint32_t object_table_offset; // Object table offset
  uint32_t num_objects;       // Number of objects in module
  // ... (many more fields)
};

// ============================================================================
// PMODE/W - PMODE DOS Extender by Charles Scheffold & Thomas Pytel
// ============================================================================

struct PMODEWHeader {
  uint16_t signature;         // 'PM' (0x4D50)
  uint16_t version;           // Version number
  uint32_t entry_point;       // 32-bit entry point
  uint32_t code_size;         // Code segment size
  uint32_t data_size;         // Data segment size
  uint32_t bss_size;          // BSS size
  uint32_t stack_size;        // Stack size
  uint16_t min_memory;        // Minimum memory (paragraphs)
  uint16_t max_memory;        // Maximum memory (paragraphs)
  uint32_t reloc_table;       // Relocation table offset
  uint16_t num_relocs;        // Number of relocations
};

// ============================================================================
// WDOSX - WD DOS Extender by Jonny Shaw
// ============================================================================

struct WDOSXStub {
  char signature[8];          // "WDOSX\x1A\x00"
  uint32_t stub_size;         // Size of DOS stub
  uint32_t image_size;        // Size of protected mode image
  uint32_t reloc_offset;      // Relocation table offset
  uint32_t reloc_count;       // Number of relocations
  uint32_t entry_point;       // Entry point offset
  uint16_t flags;             // Flags
};

// ============================================================================
// CWSDPMI - DJGPP's DOS Protected Mode Interface
// ============================================================================

struct CWSDPMIStub {
  char signature[7];          // "go32stub"
  uint8_t version;            // Stub version
  uint32_t coff_start;        // COFF executable start offset
  uint32_t initial_size;      // Initial memory allocation
  uint32_t minimum_size;      // Minimum memory needed
  uint32_t stack_size;        // Stack size
  uint32_t psp_segment;       // PSP segment
  uint16_t environment_seg;   // Environment segment
};

// ============================================================================
// Causeway DOS Extender - by Michael Devore
// ============================================================================

struct CausewayHeader {
  uint16_t signature;         // 'CW' (0x5743)
  uint16_t version;           // Extender version
  uint32_t entry_offset;      // Entry point offset
  uint32_t entry_selector;    // Entry point selector
  uint32_t code_size;         // Code size
  uint32_t data_size;         // Data size
  uint32_t bss_size;          // BSS size
  uint32_t stack_size;        // Stack size
  uint32_t image_size;        // Total image size
  uint16_t num_relocs;        // Number of relocations
  uint32_t reloc_offset;      // Relocation table offset
  uint16_t flags;             // Flags
  uint32_t auto_ds_size;      // Auto DS size
  uint32_t api_entry;         // API entry point
};

// ============================================================================
// PMODE/DJ - DJ Delorie's PMODE variant for DJGPP
// ============================================================================

struct PMODEDJHeader {
  char signature[8];          // "PMODE/DJ"
  uint16_t version;           // Version
  uint32_t load_image_size;   // Load image size
  uint32_t min_memory;        // Minimum memory required
  uint32_t max_memory;        // Maximum memory
  uint32_t load_address;      // Load address
  uint32_t entry_point;       // Entry point
  uint32_t initial_esp;       // Initial ESP
  uint32_t initial_eip;       // Initial EIP
  uint16_t cs_selector;       // CS selector
  uint16_t ss_selector;       // SS selector
  uint16_t ds_selector;       // DS selector
  uint16_t flags;             // Flags
};

// ============================================================================
// DPMI (DOS Protected Mode Interface) - Specification
// ============================================================================

struct DPMIHeader {
  uint16_t signature;         // 'SI' (0x4953)
  uint16_t last_paragraph;    // Last paragraph of real mode code
  uint16_t entry_ip;          // Entry IP in real mode
  uint16_t init_cs;           // Initial CS for protected mode
  uint16_t init_ss;           // Initial SS for protected mode
  uint16_t init_ds;           // Initial DS for protected mode
  uint16_t init_es;           // Initial ES for protected mode
  uint32_t pm_entry;          // Protected mode entry point
  uint32_t host_data;         // DPMI host private data
};

// ============================================================================
// Watcom DOS Extender - Watcom C/C++ compiler extender
// ============================================================================

struct WatcomDOSExtender {
  uint16_t signature;         // 'W4' (0x3457) for DOS/4G compatibility
  uint16_t level;             // Extender level
  uint32_t code_offset;       // Code offset
  uint32_t code_size;         // Code size
  uint32_t data_offset;       // Data offset
  uint32_t data_size;         // Data size
  uint32_t bss_size;          // BSS size
  uint32_t stack_size;        // Stack size
  uint32_t entry_point;       // Entry point
  uint16_t flags;             // Flags (flat model, etc.)
  uint32_t min_memory;        // Minimum memory
  uint32_t max_memory;        // Maximum memory
};

// ============================================================================
// Win32s - Win32 subset for Windows 3.1x
// ============================================================================

struct Win32sHeader {
  uint16_t signature;         // 'W3' (0x3357)
  uint16_t version;           // Win32s version
  uint32_t pe_offset;         // PE header offset
  uint32_t thunk_offset;      // 16-to-32 thunk offset
  uint32_t thunk_size;        // Thunk table size
  uint16_t req_win_version;   // Required Windows version
  uint16_t flags;             // Application flags
  char desc[80];              // Application description
};

// ============================================================================
// FlashTek X-32 / X-32VM - FlashTek DOS Extender
// ============================================================================

struct FlashTekX32Header {
  char signature[4];          // "X-32"
  uint16_t version;           // Version number
  uint32_t pm_entry;          // Protected mode entry
  uint32_t code_base;         // Code base address
  uint32_t code_size;         // Code size
  uint32_t data_base;         // Data base address
  uint32_t data_size;         // Data size
  uint32_t bss_size;          // BSS size
  uint32_t stack_addr;        // Stack address
  uint32_t stack_size;        // Stack size
  uint32_t min_memory;        // Minimum memory (KB)
  uint32_t max_memory;        // Maximum memory (KB)
  uint16_t flags;             // Extender flags
};

// ============================================================================
// Phar Lap 286|DOS-Extender
// ============================================================================

struct PharLap286Header {
  uint16_t signature;         // 'P2' (0x3250)
  uint16_t level;             // Executable format level
  uint16_t header_size;       // Header size in paragraphs
  uint32_t file_size;         // File size
  uint16_t checksum;          // Checksum
  uint32_t run_file_pos;      // Run file position
  uint32_t file_table_offset; // File table offset
  uint32_t file_table_size;   // File table size
  uint16_t code_selector;     // Code selector
  uint16_t data_selector;     // Data selector
  uint16_t stack_selector;    // Stack selector
  uint32_t entry_point;       // Entry point
  uint32_t stack_pointer;     // Initial SP
};

// ============================================================================
// Phar Lap 386|DOS-Extender (TNT DOS Extender)
// ============================================================================

struct PharLap386Header {
  uint16_t signature;         // 'P3' (0x3350) or 'MP' (0x504D)
  uint16_t level;             // Executable format level
  uint16_t header_size;       // Header size
  uint32_t file_size;         // File size in bytes
  uint16_t checksum;          // Checksum
  uint32_t run_file_pos;      // Run file position
  uint32_t file_table_offset; // File table offset
  uint32_t file_table_size;   // File table size
  uint32_t runtime_params;    // Runtime parameters offset
  uint32_t runtime_params_size; // Runtime parameters size
  uint32_t reloc_table_offset; // Relocation table offset
  uint32_t reloc_table_size;  // Relocation table size
  uint32_t segment_table_offset; // Segment table offset
  uint16_t num_segments;      // Number of segments
  uint16_t image_size_pages;  // Image size in pages
  uint32_t entry_offset;      // Entry offset
  uint16_t entry_selector;    // Entry selector
  uint32_t initial_esp;       // Initial ESP
  uint16_t initial_ss;        // Initial SS
  uint32_t page_size;         // Page size
  uint16_t flags;             // Flags
};

// ============================================================================
// DPMI32 - 32-bit DPMI Server
// ============================================================================

struct DPMI32Info {
  uint16_t version;           // DPMI version (major.minor)
  uint16_t flags;             // Capability flags
  uint16_t cpu_type;          // CPU type
  uint16_t pic_master;        // Master PIC base interrupt
  uint16_t pic_slave;         // Slave PIC base interrupt
  uint32_t private_data_len;  // Host private data length
};

// ============================================================================
// DOS32/A - Modern DOS extender (Adam Seychell)
// ============================================================================

struct DOS32AHeader {
  char signature[7];          // "DOS/32A"
  uint8_t version;            // Version
  uint32_t le_offset;         // LE executable offset
  uint32_t loader_size;       // Loader size
  uint32_t runtime_size;      // Runtime size
  uint16_t flags;             // Configuration flags
  uint32_t min_memory;        // Minimum memory
  uint32_t max_memory;        // Maximum memory
  uint32_t kernel_mode;       // Kernel mode flags
};

// ============================================================================
// HX DOS Extender - by Japheth
// ============================================================================

struct HXDOSExtender {
  char signature[4];          // "HXRT"
  uint16_t version;           // HX version
  uint32_t pe_offset;         // PE header offset
  uint32_t stack_size;        // Stack commit size
  uint32_t heap_size;         // Heap reserve size
  uint16_t subsystem;         // Subsystem (console/GUI)
  uint16_t flags;             // Flags
};

// ============================================================================
// WDOSX 0.97+ - Enhanced version
// ============================================================================

struct WDOSX97Header {
  char signature[8];          // "WDOSX097"
  uint32_t entry_point;       // 32-bit entry point
  uint32_t stack_size;        // Stack size
  uint32_t heap_size;         // Heap size
  uint16_t selector_code;     // Code selector
  uint16_t selector_data;     // Data selector
  uint32_t flags;             // Feature flags
};

// Extender type enumeration
enum DOSExtenderType {
  EXTENDER_NONE = 0,
  EXTENDER_DOS4G,             // DOS/4G, DOS/4GW
  EXTENDER_DOS32A,            // DOS/32 Advanced
  EXTENDER_PMODEW,            // PMODE/W
  EXTENDER_WDOSX,             // WDOSX
  EXTENDER_CWSDPMI,           // CWSDPMI (DJGPP)
  EXTENDER_CAUSEWAY,          // Causeway
  EXTENDER_PMODEDJ,           // PMODE/DJ
  EXTENDER_WATCOM,            // Watcom DOS Extender
  EXTENDER_WIN32S,            // Win32s
  EXTENDER_FLASHTEK_X32,      // FlashTek X-32
  EXTENDER_PHARLAP_286,       // Phar Lap 286|DOS-Extender
  EXTENDER_PHARLAP_386,       // Phar Lap 386|DOS-Extender (TNT)
  EXTENDER_DPMI32,            // Generic DPMI32
  EXTENDER_HX,                // HX DOS Extender
  EXTENDER_WDOSX97,           // WDOSX 0.97+
};

// Helper class for DOS extender detection and parsing
class DOSExtenderParser {
public:
  static DOSExtenderType detectExtender(const uint8_t *data, size_t size);
  static bool parseLEHeader(const uint8_t *data, size_t size, LEHeader &header);
  static bool parsePMODEWHeader(const uint8_t *data, size_t size, PMODEWHeader &header);
  static bool parseWDOSXHeader(const uint8_t *data, size_t size, WDOSXStub &header);
  static bool parseCWSDPMIStub(const uint8_t *data, size_t size, CWSDPMIStub &stub);
  static bool parseCausewayHeader(const uint8_t *data, size_t size, CausewayHeader &header);
  static bool parseWatcomHeader(const uint8_t *data, size_t size, WatcomDOSExtender &header);
  static bool parseWin32sHeader(const uint8_t *data, size_t size, Win32sHeader &header);
  static bool parsePharLap286Header(const uint8_t *data, size_t size, PharLap286Header &header);
  static bool parsePharLap386Header(const uint8_t *data, size_t size, PharLap386Header &header);
  static bool parseHXHeader(const uint8_t *data, size_t size, HXDOSExtender &header);

  static std::string getExtenderName(DOSExtenderType type);
  static uint32_t getEntryPoint(DOSExtenderType type, const uint8_t *data, size_t size);
  static bool is32BitExtender(DOSExtenderType type);
  static bool isDPMIBased(DOSExtenderType type);
};

} // namespace DOSExtenders
} // namespace BinaryFormat
} // namespace ds2
