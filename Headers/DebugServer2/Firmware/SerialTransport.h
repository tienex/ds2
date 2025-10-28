//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Unified Serial Transport for Firmware Debugging
//
// Provides cross-platform serial port communication for UEFI, OpenFirmware,
// and other firmware debugging scenarios
//

#pragma once

#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Firmware {

//
// Serial Port Configuration
//

struct SerialConfig {
  //
  // Device path
  //
  const char *device;         // /dev/ttyS0 (Linux), /dev/cu.usbserial (macOS), COM1 (Windows)

  //
  // Baud rate
  //
  uint32_t baud_rate;         // Common: 9600, 19200, 38400, 57600, 115200

  //
  // Data format
  //
  uint8_t data_bits;          // 5, 6, 7, or 8 (usually 8)
  uint8_t stop_bits;          // 1 or 2 (usually 1)
  char parity;                // 'N' (none), 'O' (odd), 'E' (even), 'M' (mark), 'S' (space)

  //
  // Flow control
  //
  bool hardware_flow;         // RTS/CTS hardware flow control
  bool software_flow;         // XON/XOFF software flow control

  //
  // Timeouts (in milliseconds)
  //
  uint32_t read_timeout;      // Read timeout (0 = no timeout)
  uint32_t write_timeout;     // Write timeout (0 = no timeout)

  //
  // Special modes
  //
  bool raw_mode;              // Raw mode (no line discipline, no echo)
  bool non_blocking;          // Non-blocking I/O

  //
  // Default configuration for firmware debugging
  //
  static SerialConfig defaultConfig(const char *device) {
    SerialConfig config;
    config.device = device;
    config.baud_rate = 115200;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = 'N';
    config.hardware_flow = false;
    config.software_flow = false;
    config.read_timeout = 1000;
    config.write_timeout = 1000;
    config.raw_mode = true;
    config.non_blocking = false;
    return config;
  }

  //
  // OpenFirmware default (typically 9600 baud)
  //
  static SerialConfig openFirmwareConfig(const char *device) {
    SerialConfig config = defaultConfig(device);
    config.baud_rate = 9600;
    return config;
  }

  //
  // UEFI default (typically 115200 baud)
  //
  static SerialConfig uefiConfig(const char *device) {
    SerialConfig config = defaultConfig(device);
    config.baud_rate = 115200;
    return config;
  }
};

//
// Serial Baud Rates
//

namespace BaudRates {
  constexpr uint32_t BAUD_110    = 110;
  constexpr uint32_t BAUD_300    = 300;
  constexpr uint32_t BAUD_600    = 600;
  constexpr uint32_t BAUD_1200   = 1200;
  constexpr uint32_t BAUD_2400   = 2400;
  constexpr uint32_t BAUD_4800   = 4800;
  constexpr uint32_t BAUD_9600   = 9600;
  constexpr uint32_t BAUD_14400  = 14400;
  constexpr uint32_t BAUD_19200  = 19200;
  constexpr uint32_t BAUD_38400  = 38400;
  constexpr uint32_t BAUD_57600  = 57600;
  constexpr uint32_t BAUD_115200 = 115200;
  constexpr uint32_t BAUD_230400 = 230400;
  constexpr uint32_t BAUD_460800 = 460800;
  constexpr uint32_t BAUD_921600 = 921600;
}

//
// Unified Serial Transport
//

class SerialTransport : public Host::Channel {
public:
  SerialTransport();
  ~SerialTransport() override;

  //
  // Channel interface implementation
  //

  void close() override;
  bool connected() const override;
  bool wait(int ms = -1) override;
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

  //
  // Connection management
  //

  ErrorCode connect(const char *device, uint32_t baud_rate = 115200);
  ErrorCode connect(const SerialConfig &config);

  //
  // Configuration
  //

  ErrorCode setBaudRate(uint32_t rate);
  ErrorCode setDataBits(uint8_t bits);
  ErrorCode setStopBits(uint8_t bits);
  ErrorCode setParity(char parity);
  ErrorCode setFlowControl(bool hardware, bool software);
  ErrorCode setTimeouts(uint32_t read_ms, uint32_t write_ms);
  ErrorCode setRawMode(bool enabled);
  ErrorCode setNonBlocking(bool enabled);

  ErrorCode reconfigure(const SerialConfig &config);
  SerialConfig getConfiguration() const { return _config; }

  //
  // Line control (DTR, RTS, etc.)
  //

  ErrorCode setDTR(bool state);
  ErrorCode setRTS(bool state);
  ErrorCode getDSR(bool *state);
  ErrorCode getCTS(bool *state);
  ErrorCode getDCD(bool *state);  // Carrier detect
  ErrorCode getRI(bool *state);   // Ring indicator

  //
  // Special operations
  //

  ErrorCode sendBreak(uint32_t duration_ms = 250);  // Send BREAK signal
  ErrorCode flush(bool input = true, bool output = true);  // Flush buffers
  ErrorCode drain();  // Wait for all output to be transmitted

  //
  // Status and information
  //

  ErrorCode getBytesAvailable(size_t *count);
  ErrorCode getBytesWritten(size_t *count);

  struct Statistics {
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t errors_parity;
    uint64_t errors_framing;
    uint64_t errors_overrun;
    uint64_t breaks_detected;
  };

  Statistics getStatistics() const { return _statistics; }
  void resetStatistics();

  //
  // Device information
  //

  struct DeviceInfo {
    char device_path[256];
    char description[256];
    char manufacturer[128];
    char serial_number[64];
    bool is_usb;
    uint16_t usb_vendor_id;
    uint16_t usb_product_id;
  };

  ErrorCode getDeviceInfo(DeviceInfo &info);

  //
  // Platform-specific device enumeration
  //

  static std::vector<std::string> enumerateDevices();
  static std::vector<DeviceInfo> enumerateDevicesWithInfo();

  //
  // Platform-specific default device
  //

  static const char* getDefaultDevice();

  //
  // Utility functions
  //

  static bool isValidBaudRate(uint32_t rate);
  static bool isValidDataBits(uint8_t bits);
  static bool isValidStopBits(uint8_t bits);
  static bool isValidParity(char parity);

private:
  //
  // Platform-specific handle
  //

#if defined(OS_WIN32)
  void *_handle;  // HANDLE (Windows)
#else
  int _fd;        // File descriptor (POSIX)
#endif

  //
  // Connection state
  //

  bool _connected;
  SerialConfig _config;
  Statistics _statistics;

  //
  // Platform-specific implementation
  //

#if defined(OS_WIN32)
  ErrorCode openWindows(const char *device);
  ErrorCode configureWindows(const SerialConfig &config);
  ErrorCode closeWindows();
  ssize_t sendWindows(void const *buffer, size_t length);
  ssize_t receiveWindows(void *buffer, size_t length);
  ErrorCode waitWindows(int ms);
  ErrorCode setLineControlWindows(bool dtr, bool rts);
  ErrorCode getLineStatusWindows(bool *dsr, bool *cts, bool *dcd, bool *ri);
  ErrorCode sendBreakWindows(uint32_t duration_ms);
  ErrorCode flushWindows(bool input, bool output);
#elif defined(OS_POSIX)
  ErrorCode openPOSIX(const char *device);
  ErrorCode configurePOSIX(const SerialConfig &config);
  ErrorCode closePOSIX();
  ssize_t sendPOSIX(void const *buffer, size_t length);
  ssize_t receivePOSIX(void *buffer, size_t length);
  ErrorCode waitPOSIX(int ms);
  ErrorCode setLineControlPOSIX(bool dtr, bool rts);
  ErrorCode getLineStatusPOSIX(bool *dsr, bool *cts, bool *dcd, bool *ri);
  ErrorCode sendBreakPOSIX(uint32_t duration_ms);
  ErrorCode flushPOSIX(bool input, bool output);
#endif

  //
  // Helper functions
  //

  ErrorCode validateConfig(const SerialConfig &config);
  void updateStatistics(bool is_send, size_t bytes);
};

//
// USB Serial Adapter Detection
//

class USBSerialDetector {
public:
  //
  // Common USB serial adapter VID/PID
  //

  struct USBDevice {
    uint16_t vendor_id;
    uint16_t product_id;
    const char *name;
  };

  static const USBDevice KNOWN_ADAPTERS[];
  static size_t KNOWN_ADAPTERS_COUNT;

  //
  // Detection
  //

  static bool isUSBSerial(const char *device);
  static bool getUSBInfo(const char *device, uint16_t *vid, uint16_t *pid);
  static const char* getAdapterName(uint16_t vid, uint16_t pid);

  //
  // Common vendors
  //

  static constexpr uint16_t VID_FTDI = 0x0403;     // FTDI (FT232, etc.)
  static constexpr uint16_t VID_PROLIFIC = 0x067B;  // Prolific (PL2303)
  static constexpr uint16_t VID_CP210X = 0x10C4;    // Silicon Labs (CP210x)
  static constexpr uint16_t VID_CH340 = 0x1A86;     // WCH (CH340)
  static constexpr uint16_t VID_APPLE = 0x05AC;     // Apple USB-Serial
};

//
// Serial Port Discovery
//

class SerialPortDiscovery {
public:
  //
  // Find firmware debugging ports
  //

  static std::vector<std::string> findUEFIDebugPorts();
  static std::vector<std::string> findOpenFirmwarePorts();
  static std::vector<std::string> findAllFirmwareDebugPorts();

  //
  // Platform-specific search paths
  //

#if defined(OS_LINUX)
  static constexpr const char* LINUX_TTY_PATHS[] = {
    "/dev/ttyS",      // Standard serial ports
    "/dev/ttyUSB",    // USB serial adapters
    "/dev/ttyACM",    // USB CDC ACM devices
    "/dev/ttyAMA",    // ARM PL011 UART (Raspberry Pi, etc.)
  };
#elif defined(OS_DARWIN)
  static constexpr const char* MACOS_TTY_PATHS[] = {
    "/dev/cu.usbserial",  // USB serial adapters
    "/dev/cu.usbmodem",   // USB CDC ACM devices
    "/dev/tty.usbserial", // USB serial adapters (alternative)
    "/dev/tty.usbmodem",  // USB CDC ACM devices (alternative)
  };
#elif defined(OS_WIN32)
  // Windows uses COM1-COM256
  static constexpr int WIN32_COM_MIN = 1;
  static constexpr int WIN32_COM_MAX = 256;
#endif

  //
  // Heuristic detection
  //

  static bool looksLikeUEFIPort(const char *device);
  static bool looksLikeOpenFirmwarePort(const char *device);

  //
  // Test connection
  //

  static bool testPort(const char *device, uint32_t baud_rate);
};

} // namespace Firmware
} // namespace ds2
