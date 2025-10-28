//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// ADB (Android Debug Bridge)
//
// ADB is Google's debugging protocol for Android devices
// Operates over USB or TCP/IP (typically port 5555)
//

#pragma once

#include "DebugServer2/Base.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ds2 {
namespace Protocol {
namespace ADB {

//
// ADB Overview
//
// ADB (Android Debug Bridge) is Google's protocol for Android debugging:
// - Communication with Android devices and emulators
// - USB or TCP/IP transport (port 5555 for wireless debugging)
// - Shell access, logcat, package installation
// - Port forwarding and reverse port forwarding
// - File synchronization (push/pull)
// - JDWP (Java Debug Wire Protocol) integration
// - Multi-device support
// - Root/non-root operation modes
//

//
// ADB Protocol Version
//

constexpr uint32_t ADB_VERSION_MIN = 0x01000000;  // ADB version 1.0.0
constexpr uint32_t ADB_VERSION_MAX = 0x01000041;  // ADB version 1.0.41
constexpr uint16_t ADB_DEFAULT_PORT = 5555;       // Default TCP port
constexpr uint16_t ADB_SERVER_PORT = 5037;        // ADB server port on host

//
// ADB Message Header
//

struct MessageHeader {
  uint32_t command;         // Command identifier (A_SYNC, A_CNXN, etc.)
  uint32_t arg0;            // First argument
  uint32_t arg1;            // Second argument
  uint32_t data_length;     // Length of payload
  uint32_t data_check;      // Checksum of payload
  uint32_t magic;           // Command ^ 0xFFFFFFFF
};

//
// ADB Commands
//

enum class Command : uint32_t {
  SYNC = 0x434E5953,  // 'SYNC' - Synchronize (connection test)
  CNXN = 0x4E584E43,  // 'CNXN' - Connect
  AUTH = 0x48545541,  // 'AUTH' - Authentication
  OPEN = 0x4E45504F,  // 'OPEN' - Open a stream
  OKAY = 0x59414B4F,  // 'OKAY' - Success/ready
  CLSE = 0x45534C43,  // 'CLSE' - Close a stream
  WRTE = 0x45545257,  // 'WRTE' - Write data to stream
};

//
// ADB Authentication Types
//

enum class AuthType : uint32_t {
  TOKEN       = 1,  // Token for signature authentication
  SIGNATURE   = 2,  // RSA signature of token
  RSAPUBLICKEY = 3, // RSA public key
};

//
// ADB Connection
//

struct ConnectionInfo {
  uint32_t version;         // ADB protocol version
  uint32_t max_data;        // Maximum data payload size
  std::string system_type;  // "host::" or "device::"
  std::string serial;       // Device serial number
  std::string banner;       // Banner string (e.g., "device::ro.product.name=sdk")
};

//
// ADB Stream
//

struct Stream {
  uint32_t local_id;        // Local stream ID
  uint32_t remote_id;       // Remote stream ID
  std::string destination;  // Destination service (e.g., "shell:ls -l")
  bool is_open;
};

//
// ADB Service Names
//

namespace Services {
  // Shell services
  constexpr const char* SHELL              = "shell:";           // Execute shell command
  constexpr const char* EXEC               = "exec:";            // Execute command (no PTY)
  constexpr const char* SHELL_V2           = "shell,v2:";        // Shell protocol v2

  // File services
  constexpr const char* SYNC               = "sync:";            // File synchronization
  constexpr const char* REVERSE            = "reverse:";         // Reverse connection

  // Logging
  constexpr const char* LOGCAT             = "logcat:";          // Android system logs

  // JDWP (Java debugging)
  constexpr const char* JDWP               = "jdwp:";            // JDWP connection
  constexpr const char* TRACK_JDWP         = "track-jdwp";       // Track JDWP processes

  // Port forwarding
  constexpr const char* TCP                = "tcp:";             // TCP port forward
  constexpr const char* LOCAL              = "local:";           // Unix socket forward
  constexpr const char* LOCALRESERVED      = "localreserved:";   // Reserved local forward
  constexpr const char* LOCALABSTRACT      = "localabstract:";   // Abstract socket forward
  constexpr const char* LOCALFILESYSTEM    = "localfilesystem:"; // Filesystem socket forward
  constexpr const char* DEV                = "dev:";             // Device file

  // Device tracking
  constexpr const char* TRACK_DEVICES      = "track-devices";    // Track device connections

  // Package management
  constexpr const char* REMOUNT            = "remount:";         // Remount /system as RW

  // Root services
  constexpr const char* ROOT               = "root:";            // Restart adbd as root
  constexpr const char* UNROOT             = "unroot:";          // Restart adbd as non-root

  // USB services
  constexpr const char* USB                = "usb:";             // USB connection
  constexpr const char* RECONNECT          = "reconnect";        // Reconnect to ADB

  // Framebuffer
  constexpr const char* FRAMEBUFFER        = "framebuffer:";     // Screen capture (raw)

  // ABB (Android Binder Bridge)
  constexpr const char* ABB                = "abb:";             // ABB command
  constexpr const char* ABB_EXEC           = "abb_exec:";        // ABB exec
}

//
// ADB Sync Commands (file transfer protocol)
//

enum class SyncCommand : uint32_t {
  STAT = 0x54415453,  // 'STAT' - Get file statistics
  LIST = 0x5453494C,  // 'LIST' - List directory
  SEND = 0x444E4553,  // 'SEND' - Send file to device
  RECV = 0x56434552,  // 'RECV' - Receive file from device
  DENT = 0x544E4544,  // 'DENT' - Directory entry
  DONE = 0x454E4F44,  // 'DONE' - Transfer complete
  DATA = 0x41544144,  // 'DATA' - File data
  OKAY = 0x59414B4F,  // 'OKAY' - Success
  FAIL = 0x4C494146,  // 'FAIL' - Failure
  QUIT = 0x54495551,  // 'QUIT' - Close sync connection
};

//
// File Statistics
//

struct FileStat {
  uint32_t mode;      // File mode (permissions)
  uint32_t size;      // File size in bytes
  uint32_t mtime;     // Modification time (Unix timestamp)
};

//
// Directory Entry
//

struct DirectoryEntry {
  uint32_t mode;      // File mode
  uint32_t size;      // File size
  uint32_t mtime;     // Modification time
  uint32_t namelen;   // Name length
  std::string name;   // File/directory name
};

//
// ADB Device Information
//

struct DeviceInfo {
  std::string serial;           // Device serial number
  std::string state;            // Device state (device, offline, bootloader, etc.)
  std::string model;            // Device model (ro.product.model)
  std::string device;           // Device codename (ro.product.device)
  std::string product;          // Product name (ro.build.product)
  std::string transport_id;     // Transport ID
  bool is_usb;                  // True if USB, false if TCP/IP
  std::string ip_address;       // IP address (if TCP/IP)
  uint16_t port;                // Port (if TCP/IP)
};

//
// Device States
//

namespace DeviceStates {
  constexpr const char* DEVICE      = "device";       // Device is online
  constexpr const char* OFFLINE     = "offline";      // Device is offline
  constexpr const char* BOOTLOADER  = "bootloader";   // Device is in bootloader/fastboot
  constexpr const char* RECOVERY    = "recovery";     // Device is in recovery mode
  constexpr const char* SIDELOAD    = "sideload";     // Device is in sideload mode
  constexpr const char* UNAUTHORIZED = "unauthorized"; // Device is unauthorized
  constexpr const char* AUTHORIZING = "authorizing";  // Device is authorizing
  constexpr const char* CONNECTING  = "connecting";   // Device is connecting
}

//
// JDWP (Java Debug Wire Protocol) Integration
//

struct JDWPProcess {
  uint32_t pid;               // Process ID
  std::string package;        // Package name
  bool is_debuggable;         // Whether app is debuggable
};

//
// Logcat Entry
//

struct LogcatEntry {
  uint32_t pid;               // Process ID
  uint32_t tid;               // Thread ID
  uint32_t sec;               // Timestamp (seconds)
  uint32_t nsec;              // Timestamp (nanoseconds)
  char priority;              // Log priority (V, D, I, W, E, F)
  std::string tag;            // Log tag
  std::string message;        // Log message
};

//
// Logcat Priorities
//

namespace LogcatPriority {
  constexpr char VERBOSE   = 'V';  // Verbose
  constexpr char DEBUG     = 'D';  // Debug
  constexpr char INFO      = 'I';  // Info
  constexpr char WARN      = 'W';  // Warning
  constexpr char ERROR     = 'E';  // Error
  constexpr char FATAL     = 'F';  // Fatal
  constexpr char SILENT    = 'S';  // Silent (suppress all)
}

//
// ADB Session
//

class Session {
public:
  Session();
  ~Session();

  //
  // Connection management
  //

  ErrorCode connectUSB(const char *serial = nullptr);
  ErrorCode connectTCP(const char *host, uint16_t port = ADB_DEFAULT_PORT);
  ErrorCode disconnect();

  //
  // Authentication
  //

  ErrorCode authenticate(const char *private_key_path);

  //
  // Stream management
  //

  ErrorCode openStream(const char *destination, uint32_t *stream_id);
  ErrorCode closeStream(uint32_t stream_id);
  ErrorCode writeStream(uint32_t stream_id, const void *data, size_t length);
  ErrorCode readStream(uint32_t stream_id, void *buffer, size_t max_length, size_t *actual_length);

  //
  // Shell commands
  //

  ErrorCode executeShell(const char *command, std::string &output);
  ErrorCode executeShellAsync(const char *command, uint32_t *stream_id);

  //
  // File operations (sync protocol)
  //

  ErrorCode stat(const char *remote_path, FileStat &stat);
  ErrorCode listDirectory(const char *remote_path, std::vector<DirectoryEntry> &entries);
  ErrorCode pushFile(const char *local_path, const char *remote_path, uint32_t mode = 0644);
  ErrorCode pullFile(const char *remote_path, const char *local_path);

  //
  // Logcat
  //

  ErrorCode startLogcat(const char *filter, uint32_t *stream_id);
  ErrorCode readLogcat(uint32_t stream_id, std::vector<LogcatEntry> &entries);

  //
  // JDWP
  //

  ErrorCode enumerateJDWPProcesses(std::vector<JDWPProcess> &processes);
  ErrorCode connectJDWP(uint32_t pid, uint32_t *stream_id);

  //
  // Port forwarding
  //

  ErrorCode forward(const char *local, const char *remote);
  ErrorCode forwardRemove(const char *local);
  ErrorCode forwardList(std::vector<std::string> &forwards);
  ErrorCode forwardRemoveAll();

  //
  // Reverse port forwarding
  //

  ErrorCode reverse(const char *remote, const char *local);
  ErrorCode reverseRemove(const char *remote);
  ErrorCode reverseList(std::vector<std::string> &reverses);
  ErrorCode reverseRemoveAll();

  //
  // Root operations
  //

  ErrorCode root();
  ErrorCode unroot();
  ErrorCode remount();

  //
  // Device information
  //

  ErrorCode getDeviceInfo(DeviceInfo &info);
  ErrorCode getSystemProperty(const char *property, std::string &value);
  ErrorCode setSystemProperty(const char *property, const char *value);

  //
  // Package management
  //

  ErrorCode installPackage(const char *apk_path, const char *options = nullptr);
  ErrorCode uninstallPackage(const char *package_name);
  ErrorCode listPackages(std::vector<std::string> &packages);

  //
  // Screen capture
  //

  ErrorCode captureScreen(void *buffer, size_t *width, size_t *height, size_t *bpp);
  ErrorCode captureScreenPNG(const char *output_path);

  //
  // State
  //

  bool isConnected() const { return _connected; }
  bool isAuthenticated() const { return _authenticated; }
  const ConnectionInfo& getConnectionInfo() const { return _connection_info; }

private:
  int _socket_fd;
  bool _connected;
  bool _authenticated;
  ConnectionInfo _connection_info;
  std::vector<Stream> _streams;
  uint32_t _next_stream_id;

  ErrorCode sendMessage(Command command, uint32_t arg0, uint32_t arg1, const void *data, size_t length);
  ErrorCode receiveMessage(MessageHeader &header, std::vector<uint8_t> &data);
  uint32_t calculateChecksum(const void *data, size_t length);
};

//
// ADB Server Commands (host side)
//

namespace HostCommands {
  // Device queries
  constexpr const char* DEVICES         = "host:devices";         // List all devices
  constexpr const char* DEVICES_L       = "host:devices-l";       // List devices (long format)
  constexpr const char* TRACK_DEVICES   = "host:track-devices";   // Track device connections

  // Connection
  constexpr const char* TRANSPORT       = "host:transport:";      // Select device by serial
  constexpr const char* TRANSPORT_USB   = "host:transport-usb";   // Select USB device
  constexpr const char* TRANSPORT_LOCAL = "host:transport-local"; // Select local (TCP) device
  constexpr const char* TRANSPORT_ANY   = "host:transport-any";   // Select any device

  // Port forwarding
  constexpr const char* FORWARD         = "host:forward:";        // Set up port forward
  constexpr const char* FORWARD_LIST    = "host:list-forward";    // List port forwards
  constexpr const char* KILLFORWARD     = "host:killforward:";    // Remove port forward
  constexpr const char* KILLFORWARD_ALL = "host:killforward-all"; // Remove all forwards

  // Server control
  constexpr const char* KILL            = "host:kill";            // Kill ADB server
  constexpr const char* VERSION         = "host:version";         // Get ADB server version

  // Device information
  constexpr const char* GET_STATE       = "host:get-state";       // Get device state
  constexpr const char* GET_SERIALNO    = "host:get-serialno";    // Get device serial
  constexpr const char* GET_DEVPATH     = "host:get-devpath";     // Get device path
}

//
// Helper Functions
//

// Parse ADB message header
ErrorCode parseMessageHeader(const void *buffer, MessageHeader &header);

// Create ADB message header
void createMessageHeader(MessageHeader &header, Command command, uint32_t arg0, uint32_t arg1, size_t data_length);

// Calculate ADB checksum
uint32_t calculateChecksum(const void *data, size_t length);

// Verify ADB magic field
bool verifyMagic(const MessageHeader &header);

// Parse device list response
ErrorCode parseDeviceList(const std::string &response, std::vector<DeviceInfo> &devices);

// Parse shell output
ErrorCode parseShellOutput(const std::vector<uint8_t> &data, std::string &output);

} // namespace ADB
} // namespace Protocol
} // namespace ds2
