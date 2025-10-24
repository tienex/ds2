//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OpenFirmware (IEEE 1275 Standard)
//
// OpenFirmware is a platform-independent boot firmware used on
// Sun SPARC, Apple PowerPC Macs, OLPC XO-1, and other systems
//

#pragma once

#include "DebugServer2/Base.h"

namespace ds2 {
namespace Firmware {
namespace OpenFirmware {

//
// OpenFirmware Overview
//
// OpenFirmware (IEEE 1275) is a platform-independent boot firmware:
// - Forth-based interactive environment
// - Device tree representation of hardware
// - Used on Sun SPARC workstations
// - Used on Apple PowerPC Macs (1994-2006)
// - Used on OLPC XO-1 laptop
// - Used on some IBM POWER systems
// - Platform-independent design
// - Interactive debugging capabilities
//

//
// OpenFirmware Cell Types
//

using cell_t = uint32_t;  // 32-bit on most systems (64-bit on some)
using phandle_t = cell_t;
using ihandle_t = cell_t;

//
// Device Tree Node
//

struct DeviceNode {
  phandle_t phandle;        // Physical handle
  char name[64];            // Node name
  char device_type[64];     // Device type property
  char compatible[256];     // Compatible property (may be comma-separated list)
  uint32_t reg[16];         // Register addresses
  uint32_t num_reg;         // Number of register entries
  char model[128];          // Model property
};

//
// OpenFirmware Package
//

struct Package {
  ihandle_t ihandle;        // Instance handle
  char name[64];            // Package name
  char args[256];           // Arguments
  bool is_open;             // Package is open
};

//
// Memory Region (from device tree)
//

struct MemoryRegion {
  uint64_t base;            // Base address
  uint64_t size;            // Size in bytes
  char name[64];            // Region name
};

//
// OpenFirmware Properties
//

struct Property {
  char name[128];           // Property name
  uint8_t value[1024];      // Property value
  uint32_t length;          // Value length in bytes
};

//
// Client Interface Services
//

enum class ServiceType {
  // Device tree services
  PEER,                     // Get peer node
  CHILD,                    // Get child node
  PARENT,                   // Get parent node
  GET_PROP_LEN,             // Get property length
  GET_PROP,                 // Get property
  NEXT_PROP,                // Get next property
  SET_PROP,                 // Set property
  FIND_DEVICE,              // Find device by path

  // Device I/O services
  OPEN,                     // Open device
  CLOSE,                    // Close device
  READ,                     // Read from device
  WRITE,                    // Write to device
  SEEK,                     // Seek in device

  // Memory services
  CLAIM,                    // Claim memory
  RELEASE,                  // Release memory
  MAP,                      // Map memory
  UNMAP,                    // Unmap memory

  // Control transfer
  BOOT,                     // Boot
  ENTER,                    // Enter OpenFirmware
  EXIT,                     // Exit OpenFirmware
  CHAIN,                    // Chain to another program
  INTERPRET,                // Interpret Forth
  CALL_METHOD,              // Call method

  // Time services
  MILLISECONDS,             // Get millisecond ticker
};

//
// OpenFirmware Debugging Interface
//

class Debug {
public:
  //
  // Client Interface Services
  //

  static ErrorCode callService(const char *service, cell_t *args, int nargs, cell_t *returns, int nreturns);

  //
  // Device Tree Navigation
  //

  static ErrorCode getPeer(phandle_t node, phandle_t *peer);
  static ErrorCode getChild(phandle_t node, phandle_t *child);
  static ErrorCode getParent(phandle_t node, phandle_t *parent);
  static ErrorCode findDevice(const char *device_path, phandle_t *node);
  static ErrorCode enumerateDevices(std::vector<DeviceNode> &devices);

  //
  // Property Access
  //

  static ErrorCode getProperty(phandle_t node, const char *name, void *value, size_t *length);
  static ErrorCode setProperty(phandle_t node, const char *name, const void *value, size_t length);
  static ErrorCode getPropertyLength(phandle_t node, const char *name, size_t *length);
  static ErrorCode nextProperty(phandle_t node, const char *previous, char *name);
  static ErrorCode enumerateProperties(phandle_t node, std::vector<Property> &properties);

  //
  // Device I/O
  //

  static ErrorCode openDevice(const char *device_path, ihandle_t *handle);
  static ErrorCode closeDevice(ihandle_t handle);
  static ErrorCode readDevice(ihandle_t handle, void *buffer, size_t length, size_t *actual);
  static ErrorCode writeDevice(ihandle_t handle, const void *buffer, size_t length, size_t *actual);
  static ErrorCode seekDevice(ihandle_t handle, uint64_t position);

  //
  // Memory Services
  //

  static ErrorCode claimMemory(uint64_t virt, uint64_t size, uint32_t align, uint64_t *base);
  static ErrorCode releaseMemory(uint64_t virt, uint64_t size);
  static ErrorCode mapMemory(uint64_t phys, uint64_t virt, uint64_t size, uint32_t mode);
  static ErrorCode unmapMemory(uint64_t virt, uint64_t size);
  static ErrorCode getMemoryMap(std::vector<MemoryRegion> &regions);

  //
  // Forth Interpreter
  //

  static ErrorCode interpret(const char *forth_code, cell_t *result);
  static ErrorCode callMethod(const char *method, ihandle_t handle, cell_t *args, int nargs, cell_t *returns, int nreturns);

  //
  // Boot and Control
  //

  static ErrorCode boot(const char *bootspec);
  static ErrorCode enterFirmware();   // Drop to OpenFirmware prompt
  static ErrorCode exitFirmware();    // Exit to OS
  static ErrorCode reset();           // Reset system
  static ErrorCode powerOff();        // Power off system

  //
  // OpenFirmware-specific information
  //

  struct SystemInfo {
    char model[128];        // System model
    char manufacturer[64];  // Manufacturer
    char serial_number[64]; // Serial number
    uint32_t clock_frequency; // CPU clock frequency
    uint32_t bus_frequency;   // Bus clock frequency
    uint32_t timebase_frequency; // Timebase frequency (PowerPC)
    char firmware_version[64];   // Firmware version
  };

  static ErrorCode getSystemInfo(SystemInfo &info);

  // Boot arguments
  static ErrorCode getBootArgs(char *args, size_t max_length);
  static ErrorCode setBootArgs(const char *args);

  // NVRAM variables
  static ErrorCode getNVRAMVariable(const char *name, char *value, size_t max_length);
  static ErrorCode setNVRAMVariable(const char *name, const char *value);
  static ErrorCode enumerateNVRAMVariables(std::vector<std::string> &names);

  // Display control (for PowerPC Macs and Sun systems)
  struct DisplayInfo {
    uint32_t width;         // Width in pixels
    uint32_t height;        // Height in pixels
    uint32_t depth;         // Color depth in bits
    uint32_t linebytes;     // Bytes per scanline
    uint64_t framebuffer;   // Framebuffer address
  };

  static ErrorCode getDisplayInfo(DisplayInfo &info);

  // Input/Output handles
  static ErrorCode getStdin(ihandle_t *handle);
  static ErrorCode getStdout(ihandle_t *handle);
  static ErrorCode getScreen(ihandle_t *handle);
};

//
// Common OpenFirmware Device Paths
//

namespace DevicePaths {
  constexpr const char* CONSOLE         = "/chosen/stdout";
  constexpr const char* KEYBOARD        = "/chosen/stdin";
  constexpr const char* SCREEN          = "/chosen/screen";
  constexpr const char* MEMORY          = "/memory";
  constexpr const char* CPU             = "/cpus";
  constexpr const char* CHOSEN          = "/chosen";
  constexpr const char* OPTIONS         = "/options";
  constexpr const char* PACKAGES        = "/packages";
  constexpr const char* ALIASES         = "/aliases";
  constexpr const char* OPENPROM        = "/openprom";
}

//
// Common Property Names
//

namespace PropertyNames {
  constexpr const char* NAME            = "name";
  constexpr const char* DEVICE_TYPE     = "device_type";
  constexpr const char* COMPATIBLE      = "compatible";
  constexpr const char* REG             = "reg";
  constexpr const char* MODEL           = "model";
  constexpr const char* STATUS          = "status";
  constexpr const char* INTERRUPTS      = "interrupts";
  constexpr const char* INTERRUPT_PARENT = "interrupt-parent";
  constexpr const char* RANGES          = "ranges";
  constexpr const char* DMA_RANGES      = "dma-ranges";
  constexpr const char* ADDRESS_CELLS   = "#address-cells";
  constexpr const char* SIZE_CELLS      = "#size-cells";
}

//
// Return Codes
//

constexpr cell_t OF_SUCCESS     = 0;
constexpr cell_t OF_FAILURE     = -1;
constexpr cell_t OF_CATCH_RESULT = -2;  // Forth exception

} // namespace OpenFirmware
} // namespace Firmware
} // namespace ds2
