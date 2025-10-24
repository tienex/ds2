//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Apple KDP (Kernel Debug Protocol)
//
// KDP is Apple's kernel debugging protocol used for macOS, iOS, watchOS, tvOS
// Operates over UDP (port 41139) or serial connection
//

#pragma once

#include "DebugServer2/Base.h"
#include <cstdint>

namespace ds2 {
namespace Protocol {
namespace KDP {

//
// KDP Overview
//
// KDP (Kernel Debug Protocol) is Apple's protocol for kernel debugging:
// - Used on macOS, iOS, iPadOS, watchOS, tvOS
// - Operates over UDP (port 41139) or serial
// - Two-machine debugging (host debugger + target machine)
// - Supports x86-64, ARM64 architectures
// - Low-level kernel debugging before system is fully booted
// - Remote kernel debugging over network
// - NMI (Non-Maskable Interrupt) triggered debugging
//

//
// KDP Packet Header
//

constexpr uint16_t KDP_REMOTE_PORT = 41139;

struct PacketHeader {
  uint8_t request;          // Request/reply type
  uint8_t is_reply;         // 0 = request, 1 = reply
  uint8_t seq;              // Sequence number
  uint8_t len;              // Length of packet (in 32-bit words)
  uint32_t key;             // Session key (for authentication)
};

//
// KDP Request/Reply Types
//

enum class RequestType : uint8_t {
  // Connection management
  CONNECT           = 0x00,  // Connect to kernel
  DISCONNECT        = 0x01,  // Disconnect from kernel
  REATTACH          = 0x02,  // Reattach to kernel
  HOSTINFO          = 0x03,  // Get host information
  VERSION           = 0x04,  // Get KDP version

  // Execution control
  SUSPEND           = 0x05,  // Suspend kernel
  RESUMECPUS        = 0x06,  // Resume CPUs

  // Memory operations
  READMEM           = 0x10,  // Read memory
  WRITEMEM          = 0x11,  // Write memory
  READMEM64         = 0x12,  // Read memory (64-bit address)
  WRITEMEM64        = 0x13,  // Write memory (64-bit address)

  // Register operations
  READREGS          = 0x14,  // Read registers
  WRITEREGS         = 0x15,  // Write registers

  // Breakpoint support
  BREAKPOINT_SET    = 0x18,  // Set breakpoint
  BREAKPOINT_REMOVE = 0x19,  // Remove breakpoint
  BREAKPOINT64_SET  = 0x1A,  // Set breakpoint (64-bit address)
  BREAKPOINT64_REMOVE = 0x1B, // Remove breakpoint (64-bit address)

  // Kernel-specific
  KERNELVERSION     = 0x20,  // Get kernel version string
  MAXBYTES          = 0x21,  // Get maximum transfer size
  WRITEREGS64       = 0x22,  // Write registers (64-bit)
  READREGS64        = 0x23,  // Read registers (64-bit)

  // CPU control
  SUSPEND_CPU       = 0x30,  // Suspend specific CPU
  RESUME_CPU        = 0x31,  // Resume specific CPU

  // Exception/panic info
  EXCEPTION         = 0x40,  // Exception occurred
  TERMINATION       = 0x41,  // Kernel termination

  // Extended operations
  DUMPINFO          = 0x50,  // Get dump information
  READIOPORT        = 0x51,  // Read I/O port (x86)
  WRITEIOPORT       = 0x52,  // Write I/O port (x86)
  READMSR           = 0x53,  // Read MSR (x86)
  WRITEMSR          = 0x54,  // Write MSR (x86)

  // Coredump support
  REBOOT            = 0x60,  // Reboot system
  HOSTREBOOT        = 0x61,  // Host reboot request
};

//
// KDP Error Codes
//

enum class ErrorCode : uint32_t {
  NONE              = 0x00000000,  // No error
  GENERAL           = 0x00000001,  // General error
  ACCESS            = 0x00000002,  // Access denied
  INVALID_ADDRESS   = 0x00000003,  // Invalid address
  INVALID_LENGTH    = 0x00000004,  // Invalid length
  INVALID_CPU       = 0x00000005,  // Invalid CPU number
  BREAKPOINT_EXISTS = 0x00000010,  // Breakpoint already exists
  BREAKPOINT_FULL   = 0x00000011,  // Breakpoint table full
  BREAKPOINT_INVALID = 0x00000012, // Invalid breakpoint
};

//
// Host Information
//

struct HostInfo {
  uint32_t cpu_mask;        // CPUs available for debugging
  uint32_t cpu_type;        // CPU type (x86-64, ARM64, etc.)
  uint32_t cpu_subtype;     // CPU subtype
};

// CPU types (from mach/machine.h)
constexpr uint32_t CPU_TYPE_X86_64 = 0x01000007;
constexpr uint32_t CPU_TYPE_ARM64  = 0x0100000C;
constexpr uint32_t CPU_TYPE_ARM64_32 = 0x0200000C;

//
// KDP Packets
//

// Connect request/reply
struct ConnectRequest {
  PacketHeader header;
  uint16_t req_reply_port;  // UDP port for replies
  uint16_t exc_note_port;   // UDP port for exception notifications
  char greeting[256];       // Greeting string
};

struct ConnectReply {
  PacketHeader header;
  ErrorCode error;
};

// Read memory request/reply
struct ReadMemRequest {
  PacketHeader header;
  uint64_t address;         // Memory address to read
  uint32_t length;          // Number of bytes to read
};

struct ReadMemReply {
  PacketHeader header;
  ErrorCode error;
  uint8_t data[1024];       // Memory data (variable length)
};

// Write memory request/reply
struct WriteMemRequest {
  PacketHeader header;
  uint64_t address;         // Memory address to write
  uint32_t length;          // Number of bytes to write
  uint8_t data[1024];       // Memory data (variable length)
};

struct WriteMemReply {
  PacketHeader header;
  ErrorCode error;
};

// Read registers request/reply
struct ReadRegsRequest {
  PacketHeader header;
  uint32_t cpu;             // CPU number
  uint32_t flavor;          // Register flavor (architecture-specific)
};

struct ReadRegsReply {
  PacketHeader header;
  ErrorCode error;
  uint8_t regs[1024];       // Register data (variable length)
};

// Set breakpoint request/reply
struct BreakpointSetRequest {
  PacketHeader header;
  uint64_t address;         // Breakpoint address
};

struct BreakpointSetReply {
  PacketHeader header;
  ErrorCode error;
};

// Version request/reply
struct VersionRequest {
  PacketHeader header;
};

struct VersionReply {
  PacketHeader header;
  uint32_t version;         // KDP protocol version
  uint32_t feature;         // Feature flags
};

// Exception notification (sent by kernel to debugger)
struct ExceptionNotification {
  PacketHeader header;
  uint32_t cpu;             // CPU that took exception
  uint32_t exception;       // Exception type
  uint32_t code;            // Exception code
  uint32_t subcode;         // Exception subcode
};

//
// KDP Session
//

class Session {
public:
  Session();
  ~Session();

  //
  // Connection management
  //

  ErrorCode connect(const char *target_ip, uint16_t port = KDP_REMOTE_PORT);
  ErrorCode disconnect();
  ErrorCode reattach();

  //
  // Information
  //

  ErrorCode getHostInfo(HostInfo &info);
  ErrorCode getVersion(uint32_t &version, uint32_t &features);
  ErrorCode getKernelVersion(char *version, size_t max_len);
  ErrorCode getMaxBytes(uint32_t &max_bytes);

  //
  // Execution control
  //

  ErrorCode suspend();
  ErrorCode resume();
  ErrorCode suspendCPU(uint32_t cpu);
  ErrorCode resumeCPU(uint32_t cpu);

  //
  // Memory operations
  //

  ErrorCode readMemory(uint64_t address, void *buffer, size_t length);
  ErrorCode writeMemory(uint64_t address, const void *buffer, size_t length);

  //
  // Register operations
  //

  ErrorCode readRegisters(uint32_t cpu, uint32_t flavor, void *regs, size_t length);
  ErrorCode writeRegisters(uint32_t cpu, uint32_t flavor, const void *regs, size_t length);

  //
  // Breakpoints
  //

  ErrorCode setBreakpoint(uint64_t address);
  ErrorCode removeBreakpoint(uint64_t address);

  //
  // Exception handling
  //

  ErrorCode waitForException(ExceptionNotification &exception, uint32_t timeout_ms);

  //
  // I/O ports (x86 only)
  //

  ErrorCode readIOPort(uint16_t port, uint32_t size, uint64_t &value);
  ErrorCode writeIOPort(uint16_t port, uint32_t size, uint64_t value);

  //
  // MSRs (x86 only)
  //

  ErrorCode readMSR(uint32_t cpu, uint32_t msr, uint64_t &value);
  ErrorCode writeMSR(uint32_t cpu, uint32_t msr, uint64_t value);

  //
  // System control
  //

  ErrorCode reboot();

private:
  int _socket_fd;
  uint32_t _session_key;
  uint8_t _seq_no;
  struct sockaddr_in _target_addr;
  struct sockaddr_in _reply_addr;
  struct sockaddr_in _exception_addr;

  ErrorCode sendPacket(const void *packet, size_t length);
  ErrorCode receivePacket(void *packet, size_t max_length, size_t *actual_length);
};

//
// KDP Register Flavors (architecture-specific)
//

namespace RegisterFlavors {
  // x86-64
  constexpr uint32_t X86_THREAD_STATE64      = 4;
  constexpr uint32_t X86_FLOAT_STATE64       = 5;
  constexpr uint32_t X86_EXCEPTION_STATE64   = 6;
  constexpr uint32_t X86_DEBUG_STATE64       = 11;
  constexpr uint32_t X86_AVX_STATE64         = 17;
  constexpr uint32_t X86_AVX512_STATE64      = 19;

  // ARM64
  constexpr uint32_t ARM_THREAD_STATE64      = 6;
  constexpr uint32_t ARM_VFP_STATE           = 7;
  constexpr uint32_t ARM_EXCEPTION_STATE64   = 8;
  constexpr uint32_t ARM_DEBUG_STATE64       = 15;
  constexpr uint32_t ARM_NEON_STATE64        = 17;
}

//
// KDP Exception Types
//

enum class ExceptionType : uint32_t {
  BAD_ACCESS        = 1,   // EXC_BAD_ACCESS
  BAD_INSTRUCTION   = 2,   // EXC_BAD_INSTRUCTION
  ARITHMETIC        = 3,   // EXC_ARITHMETIC
  EMULATION         = 4,   // EXC_EMULATION
  SOFTWARE          = 5,   // EXC_SOFTWARE
  BREAKPOINT        = 6,   // EXC_BREAKPOINT
  SYSCALL           = 7,   // EXC_SYSCALL
  MACH_SYSCALL      = 8,   // EXC_MACH_SYSCALL
  RPC_ALERT         = 9,   // EXC_RPC_ALERT
  CRASH             = 10,  // EXC_CRASH
  RESOURCE          = 11,  // EXC_RESOURCE
  GUARD             = 12,  // EXC_GUARD
  CORPSE_NOTIFY     = 13,  // EXC_CORPSE_NOTIFY
};

} // namespace KDP
} // namespace Protocol
} // namespace ds2
