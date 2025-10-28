//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UEFI (Unified Extensible Firmware Interface) Debugging
//
// UEFI is the modern replacement for BIOS on x86-64, ARM64, and other platforms
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Firmware {
namespace UEFI {

//
// UEFI Overview
//
// UEFI (Unified Extensible Firmware Interface) is the modern firmware standard:
// - Replaces legacy BIOS on modern systems
// - Supports x86-64, ARM64 (AArch64), IA-64, RISC-V
// - Modular architecture with drivers and applications
// - Secure Boot support
// - GPT (GUID Partition Table) support
// - Network boot capabilities
// - Rich pre-OS environment
//

//
// UEFI Boot Phases
//

enum class BootPhase {
  SEC,          // Security Phase (early initialization)
  PEI,          // Pre-EFI Initialization
  DXE,          // Driver Execution Environment
  BDS,          // Boot Device Selection
  TSL,          // Transient System Load (OS loader)
  RT,           // Runtime (OS is running, firmware services available)
  AL,           // After Life (shutdown/reset)
};

//
// UEFI Memory Types
//

enum class MemoryType {
  RESERVED,
  LOADER_CODE,
  LOADER_DATA,
  BOOT_SERVICES_CODE,
  BOOT_SERVICES_DATA,
  RUNTIME_SERVICES_CODE,
  RUNTIME_SERVICES_DATA,
  CONVENTIONAL,
  UNUSABLE,
  ACPI_RECLAIM,
  ACPI_NVS,
  MMIO,
  MMIO_PORT_SPACE,
  PAL_CODE,
  PERSISTENT,
};

//
// UEFI Protocol GUIDs
//

struct EFI_GUID {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t data4[8];
};

//
// Common UEFI protocols
//

namespace Protocols {
  constexpr EFI_GUID LOADED_IMAGE_PROTOCOL = {
    0x5B1B31A1, 0x9562, 0x11d2,
    {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
  };

  constexpr EFI_GUID DEVICE_PATH_PROTOCOL = {
    0x09576E91, 0x6D3F, 0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
  };

  constexpr EFI_GUID SIMPLE_TEXT_OUTPUT_PROTOCOL = {
    0x387477C2, 0x69C7, 0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
  };

  constexpr EFI_GUID GRAPHICS_OUTPUT_PROTOCOL = {
    0x9042A9DE, 0x23DC, 0x4A38,
    {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A}
  };

  constexpr EFI_GUID DEBUG_SUPPORT_PROTOCOL = {
    0x2755590C, 0x6F3C, 0x42FA,
    {0x9E, 0xA4, 0xA3, 0xBA, 0x54, 0x3C, 0xDA, 0x25}
  };
}

//
// UEFI Image Information
//

struct ImageInfo {
  uint64_t image_base;      // Base address of loaded image
  uint64_t image_size;      // Size of image in memory
  uint64_t entry_point;     // Entry point address
  char name[256];           // Image name/path
  EFI_GUID device_guid;     // Device GUID
  bool is_driver;           // True if driver, false if application
};

//
// UEFI Memory Map Entry
//

struct MemoryDescriptor {
  MemoryType type;
  uint64_t physical_start;
  uint64_t virtual_start;
  uint64_t num_pages;
  uint64_t attribute;
};

//
// UEFI Debug Support Protocol
//

struct DebugSupportProtocol {
  // Architecture-specific
  uint32_t isa;             // Instruction set architecture

  // Exception handling
  uint64_t (*GetMaxProcessorIndex)(void);
  ErrorCode (*RegisterPeriodicCallback)(uint32_t index, void *callback);
  ErrorCode (*RegisterExceptionCallback)(uint32_t index, uint32_t exception, void *callback);
  ErrorCode (*InvalidateInstructionCache)(uint32_t index, uint64_t start, uint64_t length);
};

//
// UEFI Debugging Interface
//

class Debug {
public:
  //
  // Image control
  //

  static ErrorCode loadImage(const char *path, uint64_t *image_handle);
  static ErrorCode unloadImage(uint64_t image_handle);
  static ErrorCode startImage(uint64_t image_handle);

  //
  // Memory operations
  //

  static ErrorCode readMemory(uint64_t address, void *buffer, size_t length);
  static ErrorCode writeMemory(uint64_t address, const void *buffer, size_t length);

  //
  // Register access (architecture-specific)
  //

  // x86-64
  static ErrorCode readX86_64Registers(void *regs);
  static ErrorCode writeX86_64Registers(const void *regs);

  // ARM64
  static ErrorCode readARM64Registers(void *regs);
  static ErrorCode writeARM64Registers(const void *regs);

  //
  // Breakpoint support
  //

  static ErrorCode setBreakpoint(uint64_t address);
  static ErrorCode removeBreakpoint(uint64_t address);

  //
  // Exception handling
  //

  static ErrorCode registerExceptionHandler(uint32_t exception_type, void *handler);
  static ErrorCode unregisterExceptionHandler(uint32_t exception_type);

  //
  // UEFI-specific operations
  //

  static ErrorCode getBootPhase(BootPhase &phase);
  static ErrorCode getMemoryMap(std::vector<MemoryDescriptor> &map);
  static ErrorCode enumerateLoadedImages(std::vector<ImageInfo> &images);
  static ErrorCode getImageInfo(uint64_t image_handle, ImageInfo &info);

  // Protocol access
  static ErrorCode locateProtocol(const EFI_GUID &guid, void **interface);
  static ErrorCode enumerateProtocols(std::vector<EFI_GUID> &protocols);

  // Variable access (UEFI variables are NV storage)
  static ErrorCode getVariable(const char *name, const EFI_GUID &guid,
                               void *data, size_t *data_size);
  static ErrorCode setVariable(const char *name, const EFI_GUID &guid,
                               const void *data, size_t data_size);

  // Boot options
  struct BootOption {
    uint16_t boot_number;
    char description[256];
    uint64_t file_path_list;
    bool is_active;
  };

  static ErrorCode enumerateBootOptions(std::vector<BootOption> &options);

  // Secure Boot status
  struct SecureBootInfo {
    bool is_enabled;
    bool setup_mode;
    bool audit_mode;
    bool deployed_mode;
  };

  static ErrorCode getSecureBootInfo(SecureBootInfo &info);
};

//
// UEFI Status Codes
//

namespace Status {
  constexpr uint64_t SUCCESS                  = 0;
  constexpr uint64_t LOAD_ERROR               = 0x8000000000000001ULL;
  constexpr uint64_t INVALID_PARAMETER        = 0x8000000000000002ULL;
  constexpr uint64_t UNSUPPORTED              = 0x8000000000000003ULL;
  constexpr uint64_t BAD_BUFFER_SIZE          = 0x8000000000000004ULL;
  constexpr uint64_t BUFFER_TOO_SMALL         = 0x8000000000000005ULL;
  constexpr uint64_t NOT_READY                = 0x8000000000000006ULL;
  constexpr uint64_t DEVICE_ERROR             = 0x8000000000000007ULL;
  constexpr uint64_t WRITE_PROTECTED          = 0x8000000000000008ULL;
  constexpr uint64_t OUT_OF_RESOURCES         = 0x8000000000000009ULL;
  constexpr uint64_t VOLUME_CORRUPTED         = 0x800000000000000AULL;
  constexpr uint64_t VOLUME_FULL              = 0x800000000000000BULL;
  constexpr uint64_t NO_MEDIA                 = 0x800000000000000CULL;
  constexpr uint64_t MEDIA_CHANGED            = 0x800000000000000DULL;
  constexpr uint64_t NOT_FOUND                = 0x800000000000000EULL;
  constexpr uint64_t ACCESS_DENIED            = 0x800000000000000FULL;
  constexpr uint64_t NO_RESPONSE              = 0x8000000000000010ULL;
  constexpr uint64_t NO_MAPPING               = 0x8000000000000011ULL;
  constexpr uint64_t TIMEOUT                  = 0x8000000000000012ULL;
  constexpr uint64_t NOT_STARTED              = 0x8000000000000013ULL;
  constexpr uint64_t ALREADY_STARTED          = 0x8000000000000014ULL;
  constexpr uint64_t ABORTED                  = 0x8000000000000015ULL;
  constexpr uint64_t SECURITY_VIOLATION       = 0x800000000000001AULL;
}

} // namespace UEFI
} // namespace Firmware
} // namespace ds2
