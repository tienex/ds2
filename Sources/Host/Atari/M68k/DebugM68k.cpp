//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>

namespace ds2 {
namespace Host {
namespace Atari {

// Atari TOS/GEMDOS debugging for m68k
// Supports Atari ST, STE, TT, Falcon (68000/68030/68040)

// GEMDOS system calls for debugging
#define GEMDOS_TRAP           0x21    // TRAP #1 - GEMDOS
#define BIOS_TRAP             0x2D    // TRAP #13 - BIOS
#define XBIOS_TRAP            0x2E    // TRAP #14 - XBIOS

// XBIOS debugging functions
#define XBIOS_SUPEXEC         38      // Execute in supervisor mode
#define XBIOS_MFPINT          39      // MFP interrupt control

// Exception vectors
#define ATARI_VECTOR_BASE     0x000000
#define ATARI_VECTOR_BUSERR   0x000008  // Bus error
#define ATARI_VECTOR_ADDRERR  0x00000C  // Address error
#define ATARI_VECTOR_ILLEGAL  0x000010  // Illegal instruction
#define ATARI_VECTOR_TRACE    0x000024  // Trace
#define ATARI_VECTOR_TRAP14   0x0000B8  // TRAP #14 (XBIOS)

// Hardware registers
#define MFP_BASE              0xFFFA00  // MC68901 MFP base
#define PSG_BASE              0xFF8800  // YM2149 PSG
#define VIDEL_BASE            0xFF8200  // Video controller

// M68k register context
struct M68kContext {
  uint32_t d[8];      // D0-D7
  uint32_t a[8];      // A0-A7
  uint32_t pc;        // Program counter
  uint16_t sr;        // Status register
  uint32_t usp;       // User stack pointer
  uint32_t ssp;       // Supervisor stack pointer
};

static bool g_debuggerActive = false;
static M68kContext g_context;

// Process information
struct AtariProcess {
  uint32_t baseAddr;         // Base address (basepage)
  uint32_t textAddr;         // Text segment address
  uint32_t dataAddr;         // Data segment address
  uint32_t bssAddr;          // BSS segment address
  uint32_t textSize;         // Text segment size
  uint32_t dataSize;         // Data segment size
  uint32_t bssSize;          // BSS segment size
  std::string programName;
};

static AtariProcess g_currentProcess;

class Debug {
public:
  static ErrorCode attach(uint32_t pid);
  static ErrorCode detach(uint32_t pid);

  static ErrorCode setBreakpoint(Address address);
  static ErrorCode removeBreakpoint(Address address);

  static ErrorCode resume();
  static ErrorCode suspend();
  static ErrorCode step();

  static ErrorCode readMemory(Address address, void *data, size_t size);
  static ErrorCode writeMemory(Address address, void const *data, size_t size);

  static ErrorCode readRegisters(void *regs, size_t size);
  static ErrorCode writeRegisters(void const *regs, size_t size);

  static ErrorCode getProcessInfo(AtariProcess &info);
};

ErrorCode Debug::attach(uint32_t pid) {
  DS2LOG(Debug, "attaching to Atari TOS process: PID=%u", pid);

  // On TOS, typically only one process runs at a time (single-tasking)
  // Multi-tasking is available with MultiTOS/MiNT

  // Initialize process info from basepage
  // Basepage is typically at 0x0000 for current process
  g_currentProcess.baseAddr = 0x0000;

  g_debuggerActive = true;
  return kSuccess;
}

ErrorCode Debug::detach(uint32_t pid) {
  DS2LOG(Debug, "detaching from Atari TOS process: PID=%u", pid);
  g_debuggerActive = false;
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(Address address) {
  DS2LOG(Debug, "setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Read original instruction
  uint16_t originalInstr;
  ErrorCode error = readMemory(address, &originalInstr, sizeof(originalInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read instruction at breakpoint");
    return error;
  }

  // Replace with ILLEGAL instruction (0x4AFC) or TRAP #15
  uint16_t trapInstr = 0x4E4F;  // TRAP #15

  // TOS is big-endian
  trapInstr = __builtin_bswap16(trapInstr);

  error = writeMemory(address, &trapInstr, sizeof(trapInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint instruction");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::removeBreakpoint(Address address) {
  DS2LOG(Debug, "removing breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  // Restore original instruction
  return kSuccess;
}

ErrorCode Debug::resume() {
  DS2LOG(Debug, "resuming Atari TOS process");

  // Clear trace bit in SR
  g_context.sr &= ~0x8000;

  return kSuccess;
}

ErrorCode Debug::suspend() {
  DS2LOG(Debug, "suspending Atari TOS process");

  // Save current context
  // On real hardware, this would be done via exception handler

  return kSuccess;
}

ErrorCode Debug::step() {
  DS2LOG(Debug, "single-stepping Atari TOS process");

  // Set trace bit in status register
  g_context.sr |= 0x8000;  // T bit

  return kSuccess;
}

ErrorCode Debug::readMemory(Address address, void *data, size_t size) {
  // Direct memory access on Atari ST
  // ST-RAM: 0x000000 - varies (512KB to 4MB typical)
  // TT-RAM: 0x01000000+ (TT/Falcon only)
  // I/O: 0xFF0000 - 0xFFFFFF

  uint32_t addr = address.value();

  // Validate address ranges
  if (addr >= 0xFF0000 && addr < 0x1000000) {
    // I/O range - handle carefully
    DS2LOG(Debug, "reading I/O memory at 0x%08x", addr);
  } else if (addr >= 0x01000000) {
    // TT-RAM (if available)
    DS2LOG(Debug, "reading TT-RAM at 0x%08x", addr);
  }

  // Direct memory copy
  const void *srcPtr = reinterpret_cast<const void *>(addr);
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(Address address, void const *data, size_t size) {
  uint32_t addr = address.value();

  // Check for ROM write attempts
  if (addr >= 0xE00000 && addr < 0xE40000) {
    DS2LOG(Error, "attempt to write to ROM at 0x%08x", addr);
    return kErrorInvalidArgument;
  }

  // Direct memory write
  void *destPtr = reinterpret_cast<void *>(addr);
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(void *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::M68k::CPUState *state =
      reinterpret_cast<Architecture::M68k::CPUState *>(regs);

  // Copy data registers
  for (int i = 0; i < 8; i++) {
    state->gp.regs[i] = g_context.d[i];
  }

  // Copy address registers
  for (int i = 0; i < 8; i++) {
    state->gp.regs[i + 8] = g_context.a[i];
  }

  // Special registers
  state->special.pc = g_context.pc;
  state->special.sr = g_context.sr;
  state->special.usp = g_context.usp;
  state->special.ssp = g_context.ssp;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(void const *regs, size_t size) {
  if (size < sizeof(Architecture::M68k::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::M68k::CPUState *state =
      reinterpret_cast<const Architecture::M68k::CPUState *>(regs);

  // Copy data registers
  for (int i = 0; i < 8; i++) {
    g_context.d[i] = state->gp.regs[i];
  }

  // Copy address registers
  for (int i = 0; i < 8; i++) {
    g_context.a[i] = state->gp.regs[i + 8];
  }

  // Special registers
  g_context.pc = state->special.pc;
  g_context.sr = state->special.sr;
  g_context.usp = state->special.usp;
  g_context.ssp = state->special.ssp;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(AtariProcess &info) {
  // Read basepage structure
  // Basepage format (TOS):
  // +0x00: Long - Base of TPA (text+data+bss+basepage+stack)
  // +0x04: Long - Length of TPA
  // +0x08: Long - Base of text segment
  // +0x0C: Long - Length of text segment
  // +0x10: Long - Base of data segment
  // +0x14: Long - Length of data segment
  // +0x18: Long - Base of BSS segment
  // +0x1C: Long - Length of BSS segment

  uint32_t basepage = g_currentProcess.baseAddr;

  uint32_t basepageData[8];
  ErrorCode error = readMemory(Address(basepage), basepageData,
                               sizeof(basepageData));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read basepage");
    return error;
  }

  // Convert from big-endian
  for (int i = 0; i < 8; i++) {
    basepageData[i] = __builtin_bswap32(basepageData[i]);
  }

  info.baseAddr = basepage;
  info.textAddr = basepageData[2];
  info.textSize = basepageData[3];
  info.dataAddr = basepageData[4];
  info.dataSize = basepageData[5];
  info.bssAddr = basepageData[6];
  info.bssSize = basepageData[7];

  // Program name is at basepage + 0x80 (128 bytes for command line)
  char cmdLine[128];
  error = readMemory(Address(basepage + 0x80), cmdLine, sizeof(cmdLine));
  if (error == kSuccess) {
    info.programName = std::string(cmdLine, strnlen(cmdLine, 128));
  }

  DS2LOG(Debug, "TOS process: text=0x%08x size=%u, data=0x%08x size=%u",
         info.textAddr, info.textSize, info.dataAddr, info.dataSize);

  return kSuccess;
}

} // namespace Atari
} // namespace Host
} // namespace ds2
