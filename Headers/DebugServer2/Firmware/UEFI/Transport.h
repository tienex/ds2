//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// UEFI Debugging Transport Layer
//

#pragma once

#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/Firmware/UEFI/Debug.h"

namespace ds2 {
namespace Firmware {
namespace UEFI {

//
// UEFI Debug Transport Types
//

enum class TransportType {
  EHCI_DEBUG_PORT,      // USB EHCI Debug Port (USB2 Debug Cable)
  XHCI_DEBUG_CAPABILITY,// USB XHCI Debug Capability (USB3 Debug Cable)
  SERIAL,               // Serial port (COM1/COM2/ttyS0)
  NETWORK,              // Network debugging (UDP/TCP)
  MEMORY,               // In-memory debugging (shared memory region)
  PCI,                  // PCI debug port
};

//
// EHCI Debug Port Transport
// USB2 Debug Cable using EHCI Debug Port capability
//

class EHCIDebugTransport : public Host::Channel {
public:
  EHCIDebugTransport();
  ~EHCIDebugTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // EHCI Debug Port specific
  //

  ErrorCode connect(uint16_t vendor_id = 0, uint16_t device_id = 0);
  ErrorCode setOwnership(bool take_ownership);

  struct DebugPortInfo {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint64_t base_address;
    bool     enabled;
  };

  ErrorCode getInfo(DebugPortInfo &info);

private:
  void *_debug_port_mmio;   // Memory-mapped I/O base
  bool _connected;
  bool _initialized;
  uint16_t _vendor_id;
  uint16_t _device_id;
};

//
// XHCI Debug Capability Transport
// USB3 Debug Cable using XHCI Debug Capability (DbC)
//

class XHCIDebugTransport : public Host::Channel {
public:
  XHCIDebugTransport();
  ~XHCIDebugTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // XHCI Debug Capability specific
  //

  ErrorCode connect();
  ErrorCode reset();

  struct DebugCapabilityInfo {
    uint64_t dbc_base;          // Debug Capability base address
    uint32_t dbc_offset;        // Offset in XHCI MMIO space
    bool     enabled;
    bool     halted;
    uint32_t max_packet_size;
  };

  ErrorCode getInfo(DebugCapabilityInfo &info);

private:
  void *_dbc_registers;     // Debug Capability registers
  void *_dbc_context;       // Debug Capability context
  bool _connected;
  bool _initialized;
};

//
// Serial Transport
// Traditional serial port debugging (RS-232)
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
  // Serial port specific
  //

  ErrorCode connect(const char *device, uint32_t baud_rate = 115200);

  enum class BaudRate : uint32_t {
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200,
    BAUD_230400 = 230400,
    BAUD_460800 = 460800,
    BAUD_921600 = 921600,
  };

  ErrorCode setBaudRate(BaudRate rate);
  ErrorCode setDataBits(uint8_t bits);  // 5, 6, 7, 8
  ErrorCode setParity(char parity);     // 'N', 'O', 'E', 'M', 'S'
  ErrorCode setStopBits(uint8_t bits);  // 1, 2
  ErrorCode setFlowControl(bool enabled);

  struct SerialConfig {
    const char *device;
    BaudRate baud_rate;
    uint8_t data_bits;
    char parity;
    uint8_t stop_bits;
    bool flow_control;
  };

  ErrorCode configure(const SerialConfig &config);

private:
  int _fd;                  // File descriptor (POSIX) or HANDLE (Windows)
  bool _connected;
  std::string _device_path;
};

//
// Network Transport
// Network-based UEFI debugging (UDP or TCP)
//

class NetworkTransport : public Host::Channel {
public:
  NetworkTransport();
  ~NetworkTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Network specific
  //

  ErrorCode connectTCP(const char *host, uint16_t port);
  ErrorCode connectUDP(const char *host, uint16_t port);
  ErrorCode listen(uint16_t port, bool use_tcp = true);
  ErrorCode accept();

private:
  int _socket;
  bool _connected;
  bool _is_tcp;
  std::string _remote_host;
  uint16_t _remote_port;
};

//
// Memory Transport
// Shared memory region for in-process UEFI debugging
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
  // Memory transport specific
  //

  ErrorCode connect(uint64_t base_address, size_t size);
  ErrorCode connectShared(const char *name);  // Named shared memory

  struct MemoryRegionInfo {
    uint64_t base_address;
    size_t size;
    void *mapped_address;
    bool is_shared;
    char name[256];
  };

  ErrorCode getRegionInfo(MemoryRegionInfo &info);

private:
  uint64_t _base_address;
  size_t _size;
  void *_mapped_memory;
  bool _connected;
  std::string _shared_name;

  // Ring buffer for communication
  struct RingBuffer {
    uint32_t read_offset;
    uint32_t write_offset;
    uint32_t size;
    uint8_t data[1];
  } *_ring_buffer;
};

//
// PCI Debug Port Transport
// PCI-based debug port (rare, but supported on some systems)
//

class PCIDebugTransport : public Host::Channel {
public:
  PCIDebugTransport();
  ~PCIDebugTransport() override;

  //
  // Channel interface
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // PCI Debug Port specific
  //

  ErrorCode connect(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar);

  struct PCIDebugInfo {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t bar;
    uint64_t base_address;
    uint32_t size;
    bool enabled;
  };

  ErrorCode getInfo(PCIDebugInfo &info);

private:
  void *_debug_registers;
  bool _connected;
  uint8_t _bus, _device, _function, _bar;
};

//
// Transport Factory
// Creates appropriate transport based on platform and availability
//

class TransportFactory {
public:
  //
  // Auto-detect and create best available transport
  //

  static std::unique_ptr<Host::Channel> createBestTransport();

  //
  // Create specific transport type
  //

  static std::unique_ptr<Host::Channel> createTransport(TransportType type);

  //
  // Query available transports
  //

  static std::vector<TransportType> getAvailableTransports();

  //
  // Check if specific transport is available
  //

  static bool isTransportAvailable(TransportType type);

  //
  // Get transport description
  //

  static const char* getTransportName(TransportType type);
  static const char* getTransportDescription(TransportType type);
};

//
// UEFI Debugging Session
// High-level debugging session using appropriate transport
//

class DebugSession {
public:
  DebugSession();
  ~DebugSession();

  //
  // Session management
  //

  ErrorCode connect(TransportType type = TransportType::EHCI_DEBUG_PORT);
  ErrorCode connectAuto();  // Auto-detect best transport
  ErrorCode disconnect();
  bool isConnected() const { return _connected; }

  //
  // Low-level communication
  //

  ErrorCode send(const void *data, size_t length);
  ErrorCode receive(void *data, size_t max_length, size_t *actual_length);

  //
  // High-level debugging operations
  //

  ErrorCode halt();
  ErrorCode resume();
  ErrorCode step();
  ErrorCode readMemory(uint64_t address, void *buffer, size_t length);
  ErrorCode writeMemory(uint64_t address, const void *buffer, size_t length);
  ErrorCode readRegisters(void *registers, size_t size);
  ErrorCode writeRegisters(const void *registers, size_t size);
  ErrorCode setBreakpoint(uint64_t address);
  ErrorCode removeBreakpoint(uint64_t address);

  //
  // UEFI-specific operations
  //

  ErrorCode getBootPhase(BootPhase &phase);
  ErrorCode enumerateImages(std::vector<ImageInfo> &images);
  ErrorCode getMemoryMap(std::vector<MemoryDescriptor> &map);

  //
  // Transport information
  //

  TransportType getTransportType() const { return _transport_type; }
  Host::Channel* getTransport() { return _transport.get(); }

private:
  std::unique_ptr<Host::Channel> _transport;
  TransportType _transport_type;
  bool _connected;
  bool _target_halted;
};

} // namespace UEFI
} // namespace Firmware
} // namespace ds2
