//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OpenFirmware Debugging Transport Layer
//

#pragma once

#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/Firmware/OpenFirmware/Debug.h"

namespace ds2 {
namespace Firmware {
namespace OpenFirmware {

//
// OpenFirmware Transport Types
//

enum class TransportType {
  SERIAL,           // Serial console (most common)
  NETWORK_TFTP,     // Network boot via TFTP
  NETWORK_TELNET,   // Telnet to OpenFirmware prompt
  NETWORK_UDP,      // UDP-based debugging
  MEMORY,           // Direct memory access (for hosted environments)
  DEVICE_TREE,      // Device tree interface
};

//
// Serial Console Transport
// Primary transport for OpenFirmware debugging on most platforms
//

class SerialTransport : public Host::Channel {
public:
  SerialTransport();
  ~SerialTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Serial port configuration
  //

  ErrorCode connect(const char *device, uint32_t baud_rate = 9600);

  enum class BaudRate : uint32_t {
    BAUD_1200 = 1200,     // Older Sun systems
    BAUD_2400 = 2400,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,     // Most common for OpenFirmware
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,   // PowerPC Macs
    BAUD_57600 = 57600,
    BAUD_115200 = 115200, // Modern systems
  };

  struct SerialConfig {
    const char *device;     // /dev/ttyS0, /dev/cu.usbserial, etc.
    BaudRate baud_rate;
    uint8_t data_bits;      // Usually 8
    char parity;            // Usually 'N' (none)
    uint8_t stop_bits;      // Usually 1
    bool flow_control;      // Usually false for OpenFirmware
  };

  ErrorCode configure(const SerialConfig &config);

  //
  // OpenFirmware-specific serial operations
  //

  ErrorCode sendBreak();          // Send BREAK signal to enter OF prompt
  ErrorCode setDTR(bool state);   // Set DTR line
  ErrorCode setRTS(bool state);   // Set RTS line
  ErrorCode getDSR(bool *state);  // Get DSR line state
  ErrorCode getCTS(bool *state);  // Get CTS line state

private:
  int _fd;                        // File descriptor
  bool _connected;
  std::string _device_path;
  BaudRate _baud_rate;
};

//
// Network TFTP Transport
// Network boot and debugging via TFTP
//

class TFTPTransport : public Host::Channel {
public:
  TFTPTransport();
  ~TFTPTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // TFTP specific
  //

  ErrorCode connect(const char *host, uint16_t port = 69);
  ErrorCode uploadFile(const char *local_path, const char *remote_path);
  ErrorCode downloadFile(const char *remote_path, const char *local_path);

  struct TFTPConfig {
    const char *server_ip;
    uint16_t port;
    uint32_t timeout_ms;
    uint32_t block_size;
  };

  ErrorCode configure(const TFTPConfig &config);

private:
  int _socket;
  bool _connected;
  std::string _server_ip;
  uint16_t _port;
  uint32_t _block_size;
};

//
// Telnet Transport
// Telnet connection to OpenFirmware prompt (Sun systems)
//

class TelnetTransport : public Host::Channel {
public:
  TelnetTransport();
  ~TelnetTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Telnet specific
  //

  ErrorCode connect(const char *host, uint16_t port = 23);
  ErrorCode authenticate(const char *username, const char *password);

  //
  // Telnet protocol negotiation
  //

  enum class TelnetCommand : uint8_t {
    SE   = 240,  // End of subnegotiation
    NOP  = 241,  // No operation
    DM   = 242,  // Data mark
    BRK  = 243,  // Break
    IP   = 244,  // Interrupt process
    AO   = 245,  // Abort output
    AYT  = 246,  // Are you there
    EC   = 247,  // Erase character
    EL   = 248,  // Erase line
    GA   = 249,  // Go ahead
    SB   = 250,  // Begin subnegotiation
    WILL = 251,  // Will
    WONT = 252,  // Won't
    DO   = 253,  // Do
    DONT = 254,  // Don't
    IAC  = 255,  // Interpret as command
  };

  ErrorCode sendCommand(TelnetCommand cmd);
  ErrorCode negotiate(uint8_t option, bool enable);

private:
  int _socket;
  bool _connected;
  bool _authenticated;
  std::string _remote_host;
  uint16_t _port;

  ErrorCode processOptions(const uint8_t *data, size_t length);
};

//
// UDP Transport
// UDP-based debugging protocol
//

class UDPTransport : public Host::Channel {
public:
  UDPTransport();
  ~UDPTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // UDP specific
  //

  ErrorCode connect(const char *host, uint16_t port);
  ErrorCode bind(uint16_t port);  // For receiving

private:
  int _socket;
  bool _connected;
  std::string _remote_host;
  uint16_t _remote_port;
  uint16_t _local_port;
};

//
// Memory Transport
// Direct memory access for hosted OpenFirmware environments
//

class MemoryTransport : public Host::Channel {
public:
  MemoryTransport();
  ~MemoryTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Memory access
  //

  ErrorCode connect(uint64_t of_base_address);

  //
  // OpenFirmware memory layout
  //

  struct MemoryLayout {
    uint64_t of_base;           // OpenFirmware base address
    uint64_t client_interface;  // Client interface callback
    uint64_t device_tree;       // Device tree base
    uint64_t forth_dict;        // Forth dictionary
    uint64_t memory_pool;       // Memory pool
  };

  ErrorCode getLayout(MemoryLayout &layout);

private:
  uint64_t _of_base;
  void *_mapped_memory;
  bool _connected;
  size_t _mapped_size;
};

//
// Device Tree Transport
// Device tree-based interface (Linux/device tree systems)
//

class DeviceTreeTransport : public Host::Channel {
public:
  DeviceTreeTransport();
  ~DeviceTreeTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Device tree access
  //

  ErrorCode connect(const char *dtb_path = "/proc/device-tree");

  struct DeviceTreeNode {
    char path[256];
    char name[64];
    char device_type[64];
    std::vector<std::string> compatible;
    std::vector<uint32_t> reg;
  };

  ErrorCode readProperty(const char *node_path, const char *property,
                         void *buffer, size_t *length);
  ErrorCode writeProperty(const char *node_path, const char *property,
                          const void *buffer, size_t length);
  ErrorCode enumerateNodes(const char *parent, std::vector<DeviceTreeNode> &nodes);

private:
  std::string _dtb_path;
  bool _connected;
};

//
// Transport Factory
//

class TransportFactory {
public:
  //
  // Auto-detect platform and create appropriate transport
  //

  static std::unique_ptr<Host::Channel> createBestTransport();

  //
  // Create specific transport type
  //

  static std::unique_ptr<Host::Channel> createTransport(TransportType type);

  //
  // Platform detection
  //

  enum class Platform {
    SUN_SPARC,            // Sun SPARC workstations
    APPLE_POWERPC,        // Apple PowerPC Macs
    IBM_POWER,            // IBM POWER systems
    OLPC_XO1,             // OLPC XO-1
    QEMU,                 // QEMU emulation
    UNKNOWN,
  };

  static Platform detectPlatform();

  //
  // Get recommended transport for platform
  //

  static TransportType getRecommendedTransport(Platform platform);

  //
  // Query available transports
  //

  static std::vector<TransportType> getAvailableTransports();
  static bool isTransportAvailable(TransportType type);

  //
  // Transport information
  //

  static const char* getTransportName(TransportType type);
  static const char* getTransportDescription(TransportType type);
  static const char* getPlatformName(Platform platform);
};

//
// OpenFirmware Debugging Session
//

class DebugSession {
public:
  DebugSession();
  ~DebugSession();

  //
  // Session management
  //

  ErrorCode connect(TransportType type = TransportType::SERIAL);
  ErrorCode connectAuto();  // Auto-detect platform and transport
  ErrorCode disconnect();
  bool isConnected() const { return _connected; }

  //
  // OpenFirmware prompt interaction
  //

  ErrorCode enterPrompt();     // Enter OpenFirmware prompt (if not already there)
  ErrorCode exitPrompt();      // Exit to boot process
  ErrorCode sendCommand(const char *forth_code);
  ErrorCode receiveResponse(char *buffer, size_t max_length, uint32_t timeout_ms = 1000);
  ErrorCode executeCommand(const char *forth_code, char *response, size_t max_response);

  //
  // Common OpenFirmware commands
  //

  ErrorCode printenv();                        // Show NVRAM variables
  ErrorCode setenv(const char *name, const char *value);
  ErrorCode printenv(const char *name, char *value, size_t max_length);
  ErrorCode devalias(const char *alias, char *path, size_t max_length);
  ErrorCode showdevs();                        // Show device tree
  ErrorCode probe(const char *device_path);    // Probe device
  ErrorCode words();                           // List Forth words
  ErrorCode seeWord(const char *word);         // Disassemble Forth word

  //
  // Device tree navigation
  //

  ErrorCode selectDevice(const char *path, phandle_t *phandle);
  ErrorCode getProperty(phandle_t phandle, const char *name, void *value, size_t *length);
  ErrorCode setProperty(phandle_t phandle, const char *name, const void *value, size_t length);

  //
  // Memory access
  //

  ErrorCode dump(uint64_t address, size_t length);        // Display memory
  ErrorCode fill(uint64_t address, uint8_t value, size_t length);
  ErrorCode move(uint64_t source, uint64_t dest, size_t length);
  ErrorCode read(uint64_t address, void *buffer, size_t length);
  ErrorCode write(uint64_t address, const void *buffer, size_t length);

  //
  // Boot operations
  //

  ErrorCode boot(const char *device_spec = nullptr);
  ErrorCode reset();
  ErrorCode powerOff();

  //
  // System information
  //

  struct SystemInfo {
    char banner[256];           // OpenFirmware banner
    char model[128];
    char manufacturer[64];
    char serial_number[64];
    uint32_t memory_size;
    Platform platform;
  };

  ErrorCode getSystemInfo(SystemInfo &info);

  //
  // Platform-specific commands
  //

  // Sun SPARC
  ErrorCode sunStop(char key);  // Send Stop-A sequence

  // Apple PowerPC
  ErrorCode appleNVRAMReset();  // Reset NVRAM (Command-Option-P-R)

  //
  // Transport information
  //

  TransportType getTransportType() const { return _transport_type; }
  Host::Channel* getTransport() { return _transport.get(); }

private:
  std::unique_ptr<Host::Channel> _transport;
  TransportType _transport_type;
  TransportFactory::Platform _platform;
  bool _connected;
  bool _at_prompt;

  ErrorCode waitForPrompt(uint32_t timeout_ms = 5000);
  ErrorCode sendBreakSequence();
};

//
// Platform-Specific Helper Functions
//

namespace PlatformHelpers {
  // Sun SPARC
  namespace Sun {
    ErrorCode sendStopA(SerialTransport *transport);
    ErrorCode sendL1A(SerialTransport *transport);
    bool detectSunSystem();
  }

  // Apple PowerPC
  namespace Apple {
    ErrorCode sendCommandOptionPR(SerialTransport *transport);
    ErrorCode sendCommandOptionOF(SerialTransport *transport);
    bool detectAppleSystem();
    const char* getAppleSerialDevice();  // Usually /dev/cu.usbserial or /dev/tty.usbserial
  }

  // IBM POWER
  namespace IBM {
    ErrorCode enterSMS(SerialTransport *transport);  // System Management Services
    bool detectIBMSystem();
  }

  // OLPC
  namespace OLPC {
    bool detectOLPCSystem();
    const char* getOLPCSerialDevice();
  }
}

} // namespace OpenFirmware
} // namespace Firmware
} // namespace ds2
