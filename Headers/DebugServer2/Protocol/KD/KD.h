//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Microsoft KD (Kernel Debugger) Protocol
//
// KD is Microsoft's kernel debugging protocol for Windows
// Used by WinDbg, kd.exe for remote kernel debugging
//

#pragma once

#include "DebugServer2/Base.h"
#include <cstdint>

namespace ds2 {
namespace Protocol {
namespace KD {

//
// KD Protocol Overview
//
// KD (Kernel Debugger) is Microsoft's protocol for Windows kernel debugging:
// - Used by WinDbg, kd.exe for remote kernel debugging
// - Operates over serial (COM), USB, 1394 (FireWire), network (NET)
// - Two-machine debugging (host debugger + target machine)
// - Supports x86, x64, ARM, ARM64 architectures
// - Kernel and hypervisor debugging
// - Live kernel debugging (non-invasive)
// - Dump file analysis
//

//
// KD Packet Types
//

constexpr uint32_t PACKET_LEADER        = 0x30303030;  // "0000"
constexpr uint32_t CONTROL_PACKET_LEADER = 0x69696969; // "iiii"
constexpr uint32_t BREAKIN_PACKET_BYTE  = 0x62;        // "b"

constexpr uint16_t PACKET_TYPE_UNUSED           = 0;
constexpr uint16_t PACKET_TYPE_KD_STATE_CHANGE32 = 1;
constexpr uint16_t PACKET_TYPE_KD_STATE_MANIPULATE = 2;
constexpr uint16_t PACKET_TYPE_KD_DEBUG_IO       = 3;
constexpr uint16_t PACKET_TYPE_KD_ACKNOWLEDGE    = 4;
constexpr uint16_t PACKET_TYPE_KD_RESEND         = 5;
constexpr uint16_t PACKET_TYPE_KD_RESET          = 6;
constexpr uint16_t PACKET_TYPE_KD_STATE_CHANGE64 = 7;
constexpr uint16_t PACKET_TYPE_KD_POLL_BREAKIN   = 8;
constexpr uint16_t PACKET_TYPE_KD_TRACE_IO       = 9;
constexpr uint16_t PACKET_TYPE_KD_CONTROL_REQUEST = 10;
constexpr uint16_t PACKET_TYPE_KD_FILE_IO        = 11;

//
// KD Packet Header
//

struct PacketHeader {
  uint32_t leader;          // Packet leader (PACKET_LEADER)
  uint16_t packet_type;     // Packet type
  uint16_t byte_count;      // Data byte count
  uint32_t packet_id;       // Packet ID
  uint32_t checksum;        // Data checksum
};

//
// KD API Numbers (for STATE_MANIPULATE packets)
//

enum class ApiNumber : uint32_t {
  READ_VIRTUAL_MEMORY        = 0x00003130,
  WRITE_VIRTUAL_MEMORY       = 0x00003131,
  GET_CONTEXT                = 0x00003132,
  SET_CONTEXT                = 0x00003133,
  WRITE_BREAKPOINT           = 0x00003134,
  RESTORE_BREAKPOINT         = 0x00003135,
  CONTINUE                   = 0x00003136,
  READ_CONTROL_SPACE         = 0x00003137,
  WRITE_CONTROL_SPACE        = 0x00003138,
  READ_IO_SPACE              = 0x00003139,
  WRITE_IO_SPACE             = 0x0000313A,
  REBOOT                     = 0x0000313B,
  CONTINUE2                  = 0x0000313C,
  READ_PHYSICAL_MEMORY       = 0x0000313D,
  WRITE_PHYSICAL_MEMORY      = 0x0000313E,
  QUERY_SPECIAL_CALLS        = 0x0000313F,
  SET_SPECIAL_CALL           = 0x00003140,
  CLEAR_SPECIAL_CALLS        = 0x00003141,
  SET_INTERNAL_BREAKPOINT    = 0x00003142,
  GET_INTERNAL_BREAKPOINT    = 0x00003143,
  READ_IO_SPACE_EXTENDED     = 0x00003144,
  WRITE_IO_SPACE_EXTENDED    = 0x00003145,
  GET_VERSION                = 0x00003146,
  WRITE_BREAKPOINT_EX        = 0x00003147,
  RESTORE_BREAKPOINT_EX      = 0x00003148,
  CAUSE_BUGCHECK             = 0x00003149,
  SWITCH_PROCESSOR           = 0x00003150,
  SEARCH_MEMORY              = 0x00003151,
  FILL_MEMORY                = 0x00003152,
  QUERY_MEMORY               = 0x00003153,
  SWITCH_PARTITION           = 0x00003154,
  READ_MACHINE_SPECIFIC_REGISTER = 0x00003155,
  WRITE_MACHINE_SPECIFIC_REGISTER = 0x00003156,
};

//
// State Change Types
//

enum class StateChange : uint32_t {
  EXCEPTION        = 0x00003030,
  LOAD_SYMBOLS     = 0x00003031,
  COMMAND_STRING   = 0x00003032,
};

//
// Exception Record (32-bit)
//

struct ExceptionRecord32 {
  uint32_t exception_code;
  uint32_t exception_flags;
  uint32_t exception_record;  // Pointer to nested exception
  uint32_t exception_address;
  uint32_t number_parameters;
  uint32_t exception_information[15];
};

//
// Exception Record (64-bit)
//

struct ExceptionRecord64 {
  uint32_t exception_code;
  uint32_t exception_flags;
  uint64_t exception_record;  // Pointer to nested exception
  uint64_t exception_address;
  uint32_t number_parameters;
  uint32_t unused_alignment;
  uint64_t exception_information[15];
};

//
// State Change Packet (64-bit)
//

struct StateChange64 {
  uint32_t new_state;         // StateChange type
  uint16_t processor_level;
  uint16_t processor;
  uint32_t number_processors;
  uint64_t thread;
  uint64_t program_counter;
  // Exception-specific data follows
  ExceptionRecord64 exception;
  uint32_t first_chance;
};

//
// State Manipulate Packet
//

struct StateManipulate {
  uint32_t api_number;        // ApiNumber
  uint16_t processor_level;
  uint16_t processor;
  uint32_t return_status;     // NTSTATUS
  // API-specific data follows
};

//
// Read Memory Request/Reply
//

struct ReadMemoryRequest {
  uint64_t target_base_address;
  uint32_t transfer_count;
  uint32_t actual_bytes_read;
};

struct ReadMemoryReply {
  uint64_t target_base_address;
  uint32_t transfer_count;
  uint32_t actual_bytes_read;
  uint8_t data[4096];         // Variable length
};

//
// Write Memory Request/Reply
//

struct WriteMemoryRequest {
  uint64_t target_base_address;
  uint32_t transfer_count;
  uint32_t actual_bytes_written;
  uint8_t data[4096];         // Variable length
};

struct WriteMemoryReply {
  uint64_t target_base_address;
  uint32_t transfer_count;
  uint32_t actual_bytes_written;
};

//
// Get Context Request/Reply
//

struct GetContextRequest {
  uint32_t context_flags;
};

struct GetContextReply {
  uint32_t context_flags;
  uint8_t context_data[4096]; // Architecture-specific CONTEXT structure
};

//
// Set Context Request/Reply
//

struct SetContextRequest {
  uint32_t context_flags;
  uint8_t context_data[4096]; // Architecture-specific CONTEXT structure
};

struct SetContextReply {
  uint32_t context_flags;
};

//
// Write Breakpoint Request/Reply
//

struct WriteBreakpointRequest {
  uint64_t breakpoint_address;
  uint32_t breakpoint_handle; // Output: assigned handle
};

struct WriteBreakpointReply {
  uint64_t breakpoint_address;
  uint32_t breakpoint_handle;
};

//
// Continue Request
//

struct ContinueRequest {
  uint32_t continue_status;   // DBG_CONTINUE or DBG_EXCEPTION_NOT_HANDLED
};

//
// Get Version Request/Reply
//

struct GetVersionRequest {
  // No specific request data
};

struct GetVersionReply {
  uint16_t major_version;
  uint16_t minor_version;
  uint16_t protocol_version;
  uint16_t flags;
  uint16_t machine_type;      // IMAGE_FILE_MACHINE_*
  uint8_t max_packet_type;
  uint8_t max_state_change;
  uint8_t max_manipulate;
  uint8_t simulation;
  uint16_t unused[1];
  uint64_t kernel_base;
  uint64_t psloaded_module_list;
  uint64_t debugger_data_list;
};

//
// Debug I/O Packet
//

struct DebugIOPacket {
  uint32_t api_number;        // API type
  uint16_t processor_level;
  uint16_t processor;
  union {
    struct {
      uint32_t length_of_string;
      uint8_t string[512];    // Debug output string
    } print_string;
    struct {
      uint32_t length_of_prompt_string;
      uint32_t length_of_string_read;
      uint8_t prompt_string[512];
    } get_string;
  } u;
};

//
// Machine Types (from winnt.h IMAGE_FILE_MACHINE_*)
//

constexpr uint16_t MACHINE_I386         = 0x014c;  // x86
constexpr uint16_t MACHINE_AMD64        = 0x8664;  // x64
constexpr uint16_t MACHINE_ARM          = 0x01c0;  // ARM (little endian)
constexpr uint16_t MACHINE_ARM64        = 0xaa64;  // ARM64 (little endian)
constexpr uint16_t MACHINE_ARMNT        = 0x01c4;  // ARM Thumb-2 (little endian)

//
// Exception Codes
//

constexpr uint32_t STATUS_BREAKPOINT               = 0x80000003;
constexpr uint32_t STATUS_SINGLE_STEP              = 0x80000004;
constexpr uint32_t STATUS_ACCESS_VIOLATION         = 0xC0000005;
constexpr uint32_t STATUS_IN_PAGE_ERROR            = 0xC0000006;
constexpr uint32_t STATUS_INVALID_HANDLE           = 0xC0000008;
constexpr uint32_t STATUS_NO_MEMORY                = 0xC0000017;
constexpr uint32_t STATUS_ILLEGAL_INSTRUCTION      = 0xC000001D;
constexpr uint32_t STATUS_NONCONTINUABLE_EXCEPTION = 0xC0000025;
constexpr uint32_t STATUS_INVALID_DISPOSITION      = 0xC0000026;
constexpr uint32_t STATUS_ARRAY_BOUNDS_EXCEEDED    = 0xC000008C;
constexpr uint32_t STATUS_FLOAT_DENORMAL_OPERAND   = 0xC000008D;
constexpr uint32_t STATUS_FLOAT_DIVIDE_BY_ZERO     = 0xC000008E;
constexpr uint32_t STATUS_INTEGER_DIVIDE_BY_ZERO   = 0xC0000094;
constexpr uint32_t STATUS_INTEGER_OVERFLOW         = 0xC0000095;
constexpr uint32_t STATUS_PRIVILEGED_INSTRUCTION   = 0xC0000096;
constexpr uint32_t STATUS_STACK_OVERFLOW           = 0xC00000FD;
constexpr uint32_t STATUS_CONTROL_C_EXIT           = 0xC000013A;

//
// Continue Status
//

constexpr uint32_t DBG_CONTINUE                    = 0x00010002;
constexpr uint32_t DBG_EXCEPTION_NOT_HANDLED       = 0x80010001;

//
// KD Session
//

class Session {
public:
  Session();
  ~Session();

  //
  // Connection management
  //

  ErrorCode connectSerial(const char *port, uint32_t baud_rate);
  ErrorCode connectNetwork(const char *target_ip, uint16_t port);
  ErrorCode disconnect();

  //
  // Initial handshake
  //

  ErrorCode sendBreakIn();
  ErrorCode waitForStateChange(StateChange64 &state, uint32_t timeout_ms);

  //
  // Version and information
  //

  ErrorCode getVersion(GetVersionReply &version);

  //
  // Execution control
  //

  ErrorCode continueExecution(uint32_t continue_status);
  ErrorCode sendCtrlC();  // Break into debugger

  //
  // Memory operations
  //

  ErrorCode readVirtualMemory(uint64_t address, void *buffer, uint32_t length, uint32_t *bytes_read);
  ErrorCode writeVirtualMemory(uint64_t address, const void *buffer, uint32_t length, uint32_t *bytes_written);
  ErrorCode readPhysicalMemory(uint64_t address, void *buffer, uint32_t length, uint32_t *bytes_read);
  ErrorCode writePhysicalMemory(uint64_t address, const void *buffer, uint32_t length, uint32_t *bytes_written);

  //
  // Register operations (context)
  //

  ErrorCode getContext(uint32_t context_flags, void *context, size_t context_size);
  ErrorCode setContext(uint32_t context_flags, const void *context, size_t context_size);

  //
  // Breakpoints
  //

  ErrorCode setBreakpoint(uint64_t address, uint32_t *handle);
  ErrorCode removeBreakpoint(uint32_t handle);

  //
  // Processor control
  //

  ErrorCode switchProcessor(uint16_t processor);
  ErrorCode getCurrentProcessor(uint16_t *processor);

  //
  // I/O operations (x86/x64 only)
  //

  ErrorCode readIOSpace(uint32_t interface_type, uint32_t bus_number, uint32_t address_space,
                        uint64_t io_address, uint32_t data_size, uint32_t *data);
  ErrorCode writeIOSpace(uint32_t interface_type, uint32_t bus_number, uint32_t address_space,
                         uint64_t io_address, uint32_t data_size, uint32_t data);

  //
  // MSR operations (x86/x64 only)
  //

  ErrorCode readMSR(uint32_t msr, uint64_t *value);
  ErrorCode writeMSR(uint32_t msr, uint64_t value);

  //
  // Control space operations
  //

  ErrorCode readControlSpace(uint16_t processor, uint64_t address, void *buffer, uint32_t length);
  ErrorCode writeControlSpace(uint16_t processor, uint64_t address, const void *buffer, uint32_t length);

  //
  // Debug output
  //

  ErrorCode readDebugOutput(char *buffer, size_t max_length, uint32_t timeout_ms);

  //
  // System control
  //

  ErrorCode reboot();
  ErrorCode causeBugCheck();  // Force a bug check (crash dump)

private:
  int _transport_fd;          // Serial or network connection
  uint32_t _packet_id_expected;
  uint32_t _packet_id_sent;
  uint16_t _current_processor;
  bool _is_64bit;

  ErrorCode sendPacket(uint16_t packet_type, const void *data, size_t length);
  ErrorCode receivePacket(uint16_t *packet_type, void *data, size_t max_length, size_t *actual_length);
  ErrorCode sendControlPacket(uint16_t packet_type);
  uint32_t calculateChecksum(const void *data, size_t length);
};

//
// Context Flags (for GetContext/SetContext)
//

namespace ContextFlags {
  // x86
  constexpr uint32_t I386_CONTEXT_i386             = 0x00010000;
  constexpr uint32_t I386_CONTEXT_CONTROL          = 0x00010001;
  constexpr uint32_t I386_CONTEXT_INTEGER          = 0x00010002;
  constexpr uint32_t I386_CONTEXT_SEGMENTS         = 0x00010004;
  constexpr uint32_t I386_CONTEXT_FLOATING_POINT   = 0x00010008;
  constexpr uint32_t I386_CONTEXT_DEBUG_REGISTERS  = 0x00010010;
  constexpr uint32_t I386_CONTEXT_EXTENDED_REGISTERS = 0x00010020;
  constexpr uint32_t I386_CONTEXT_FULL             = 0x00010007;
  constexpr uint32_t I386_CONTEXT_ALL              = 0x0001003F;

  // x64
  constexpr uint32_t AMD64_CONTEXT_AMD64           = 0x00100000;
  constexpr uint32_t AMD64_CONTEXT_CONTROL         = 0x00100001;
  constexpr uint32_t AMD64_CONTEXT_INTEGER         = 0x00100002;
  constexpr uint32_t AMD64_CONTEXT_SEGMENTS        = 0x00100004;
  constexpr uint32_t AMD64_CONTEXT_FLOATING_POINT  = 0x00100008;
  constexpr uint32_t AMD64_CONTEXT_DEBUG_REGISTERS = 0x00100010;
  constexpr uint32_t AMD64_CONTEXT_FULL            = 0x00100007;
  constexpr uint32_t AMD64_CONTEXT_ALL             = 0x0010001F;

  // ARM64
  constexpr uint32_t ARM64_CONTEXT_ARM64           = 0x00400000;
  constexpr uint32_t ARM64_CONTEXT_CONTROL         = 0x00400001;
  constexpr uint32_t ARM64_CONTEXT_INTEGER         = 0x00400002;
  constexpr uint32_t ARM64_CONTEXT_FLOATING_POINT  = 0x00400004;
  constexpr uint32_t ARM64_CONTEXT_DEBUG_REGISTERS = 0x00400008;
  constexpr uint32_t ARM64_CONTEXT_FULL            = 0x00400007;
  constexpr uint32_t ARM64_CONTEXT_ALL             = 0x0040000F;
}

} // namespace KD
} // namespace Protocol
} // namespace ds2
