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
#include <string>
#include <vector>

namespace ds2 {
namespace Host {
namespace DOS {

// DOS Debugging Support
// Real mode (8086/8088) and Protected mode (286/386+ with DOS extenders)
// Uses INT 1 (single step), INT 3 (breakpoint), and various debugging APIs

// PSP (Program Segment Prefix) structure
struct PSP {
  uint16_t int20_opcode;      // INT 20h instruction (terminate)
  uint16_t memory_top;        // Segment of top of memory
  uint8_t reserved1;          // Reserved
  uint8_t dos_dispatcher[5];  // Far call to DOS function dispatcher
  uint32_t terminate_vector;  // INT 22h terminate address
  uint32_t ctrl_break_vector; // INT 23h Ctrl-Break handler address
  uint32_t critical_error_vector; // INT 24h critical error handler
  uint16_t parent_psp;        // Parent PSP segment
  uint8_t jft[20];            // Job File Table
  uint16_t environment_seg;   // Environment segment
  uint32_t ss_sp;             // SS:SP on entry to last INT 21h
  uint16_t jft_size;          // Job File Table size
  uint32_t jft_pointer;       // Pointer to JFT
  uint32_t prev_psp;          // Previous PSP (INT 21h, AH=50h/51h)
  uint8_t reserved2[4];       // Reserved
  uint8_t dos_version[2];     // DOS version to return
  uint8_t reserved3[14];      // Reserved
  uint8_t dos_call[3];        // INT 21h + RETF instruction
  uint8_t reserved4[9];       // Reserved
  uint8_t fcb1[16];           // FCB 1
  uint8_t fcb2[20];           // FCB 2 (overlap with command tail)
  uint8_t command_tail_len;   // Length of command tail
  char command_tail[127];     // Command tail
};

// DOS Memory Control Block (MCB)
struct MCB {
  uint8_t type;               // 'M' = middle block, 'Z' = last block
  uint16_t owner_psp;         // Owner PSP segment (0 = free)
  uint16_t size;              // Size in paragraphs (16-byte blocks)
  uint8_t reserved[3];        // Reserved
  char program_name[8];       // Program name (DOS 4+)
};

// Debug registers (386+)
struct DebugRegisters {
  uint32_t dr0;               // Breakpoint address 0
  uint32_t dr1;               // Breakpoint address 1
  uint32_t dr2;               // Breakpoint address 2
  uint32_t dr3;               // Breakpoint address 3
  uint32_t dr4;               // Reserved (aliased to dr6)
  uint32_t dr5;               // Reserved (aliased to dr7)
  uint32_t dr6;               // Debug status
  uint32_t dr7;               // Debug control
};

// DR6 (Debug Status Register) bits
enum DR6Bits {
  DR6_B0 = (1 << 0),          // Breakpoint 0 triggered
  DR6_B1 = (1 << 1),          // Breakpoint 1 triggered
  DR6_B2 = (1 << 2),          // Breakpoint 2 triggered
  DR6_B3 = (1 << 3),          // Breakpoint 3 triggered
  DR6_BD = (1 << 13),         // Debug register access detected
  DR6_BS = (1 << 14),         // Single step
  DR6_BT = (1 << 15),         // Task switch
};

// DR7 (Debug Control Register) bits
enum DR7Bits {
  DR7_L0 = (1 << 0),          // Local BP 0 enable
  DR7_G0 = (1 << 1),          // Global BP 0 enable
  DR7_L1 = (1 << 2),          // Local BP 1 enable
  DR7_G1 = (1 << 3),          // Global BP 1 enable
  DR7_L2 = (1 << 4),          // Local BP 2 enable
  DR7_G2 = (1 << 5),          // Global BP 2 enable
  DR7_L3 = (1 << 6),          // Local BP 3 enable
  DR7_G3 = (1 << 7),          // Global BP 3 enable
  DR7_LE = (1 << 8),          // Local exact breakpoint enable
  DR7_GE = (1 << 9),          // Global exact breakpoint enable
  DR7_GD = (1 << 13),         // General detect enable
};

// Watchpoint types for DR7
enum WatchpointType {
  WATCH_EXEC = 0,             // Execution
  WATCH_WRITE = 1,            // Write
  WATCH_IO = 2,               // I/O (not widely supported)
  WATCH_READ_WRITE = 3,       // Read or write
};

// Watchpoint sizes for DR7
enum WatchpointSize {
  WATCH_1_BYTE = 0,
  WATCH_2_BYTES = 1,
  WATCH_8_BYTES = 2,
  WATCH_4_BYTES = 3,
};

// DOS extender modes
enum DOSMode {
  DOS_REAL_MODE = 0,          // Real mode (8086/8088)
  DOS_V86_MODE = 1,           // Virtual 8086 mode
  DOS_PROTECTED_16 = 2,       // 16-bit protected mode (286)
  DOS_PROTECTED_32 = 3,       // 32-bit protected mode (386+)
};

// DPMI (DOS Protected Mode Interface) structures
struct DPMIVersion {
  uint8_t major;              // Major version
  uint8_t minor;              // Minor version
  uint16_t flags;             // Capability flags
  uint8_t cpu_type;           // CPU type
  uint8_t pic_master;         // Master PIC base
  uint8_t pic_slave;          // Slave PIC base
};

// Debug operations
class Debug {
public:
  Debug();
  ~Debug();

public:
  // Mode detection
  static ErrorCode detectMode(DOSMode &mode);
  static bool isDPMIAvailable();
  static ErrorCode getDPMIVersion(DPMIVersion &version);

  // Process control (DOS program execution)
  static ErrorCode loadProgram(const char *path, uint16_t &psp_segment);
  static ErrorCode terminateProgram(uint16_t psp_segment);

  // Execution control
  static ErrorCode run();
  static ErrorCode singleStep();
  static ErrorCode runUntil(uint16_t segment, uint16_t offset);

  // Breakpoints (INT 3)
  static ErrorCode setBreakpoint(uint16_t segment, uint16_t offset);
  static ErrorCode clearBreakpoint(uint16_t segment, uint16_t offset);

  // Hardware breakpoints/watchpoints (386+ debug registers)
  static ErrorCode setHardwareBreakpoint(uint8_t index, uint32_t address,
                                        WatchpointType type, WatchpointSize size);
  static ErrorCode clearHardwareBreakpoint(uint8_t index);
  static ErrorCode getDebugRegisters(DebugRegisters &regs);
  static ErrorCode setDebugRegisters(const DebugRegisters &regs);

  // Memory operations (real mode - segment:offset)
  static ErrorCode readMemory(uint16_t segment, uint16_t offset,
                             void *data, size_t size);
  static ErrorCode writeMemory(uint16_t segment, uint16_t offset,
                              const void *data, size_t size);

  // Protected mode memory operations (linear addressing)
  static ErrorCode readLinearMemory(uint32_t address, void *data, size_t size);
  static ErrorCode writeLinearMemory(uint32_t address, const void *data, size_t size);

  // Register access (via INT 1 trap)
  static ErrorCode getRegisters(void *registers, size_t size);
  static ErrorCode setRegisters(const void *registers, size_t size);

  // PSP and MCB inspection
  static ErrorCode readPSP(uint16_t psp_segment, PSP &psp);
  static ErrorCode enumerateMemoryBlocks(std::vector<MCB> &blocks);
  static ErrorCode getEnvironment(uint16_t psp_segment, std::string &env);

  // DOS extender support
  static ErrorCode getDPMIHostInfo(void *buffer, size_t size);
  static ErrorCode allocateDOSMemory(uint16_t paragraphs, uint16_t &segment);
  static ErrorCode freeDOSMemory(uint16_t segment);

  // Interrupt vector management
  static ErrorCode getInterruptVector(uint8_t interrupt, uint32_t &address);
  static ErrorCode setInterruptVector(uint8_t interrupt, uint32_t address);

  // CPU state
  static ErrorCode getCPUType(uint8_t &cpu_type);
  static ErrorCode getFPUType(uint8_t &fpu_type);

private:
  static bool _initialized;
  static DOSMode _current_mode;
};

} // namespace DOS
} // namespace Host
} // namespace ds2
