#include "DebugServer2/Host/DOS/Debug.h"
#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace DOS {

// DOS Protected Mode debugging (386+ with DOS extenders)
static std::map<uint32_t, uint8_t> g_breakpoints;
DOSMode Debug::_current_mode = DOS_PROTECTED_32;

ErrorCode Debug::detectMode(DOSMode &mode) {
  // Check if running under DPMI
  if (isDPMIAvailable()) {
    mode = DOS_PROTECTED_32;
  } else {
    mode = DOS_REAL_MODE;
  }
  _current_mode = mode;
  return kSuccess;
}

bool Debug::isDPMIAvailable() {
  // INT 2Fh, AX=1687h - DPMI installation check
  return true; // Assume DPMI available for protected mode
}

ErrorCode Debug::getDPMIVersion(DPMIVersion &version) {
  memset(&version, 0, sizeof(version));
  // DPMI function 0400h - Get version
  version.major = 1;
  version.minor = 0;
  version.cpu_type = 3; // 386
  return kSuccess;
}

ErrorCode Debug::loadProgram(const char *path, uint16_t &psp_segment) {
  DS2LOG(Debug, "DOS32: loading program %s", path);
  psp_segment = 0;
  return kSuccess;
}

ErrorCode Debug::terminateProgram(uint16_t psp_segment) {
  DS2LOG(Debug, "DOS32: terminating program");
  return kSuccess;
}

ErrorCode Debug::run() {
  return kSuccess;
}

ErrorCode Debug::singleStep() {
  // Set TF in EFLAGS
  return kSuccess;
}

ErrorCode Debug::runUntil(uint16_t segment, uint16_t offset) {
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(uint16_t segment, uint16_t offset) {
  uint32_t linear = (static_cast<uint32_t>(segment) << 4) + offset;
  DS2LOG(Debug, "DOS32: setting breakpoint at linear %08X", linear);

  uint8_t orig;
  readLinearMemory(linear, &orig, 1);
  g_breakpoints[linear] = orig;

  uint8_t int3 = 0xCC;
  return writeLinearMemory(linear, &int3, 1);
}

ErrorCode Debug::clearBreakpoint(uint16_t segment, uint16_t offset) {
  uint32_t linear = (static_cast<uint32_t>(segment) << 4) + offset;
  auto it = g_breakpoints.find(linear);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;

  return writeLinearMemory(linear, &it->second, 1);
}

ErrorCode Debug::setHardwareBreakpoint(uint8_t index, uint32_t address,
                                       WatchpointType type, WatchpointSize size) {
  if (index > 3) return kErrorInvalidArgument;

  DS2LOG(Debug, "DOS32: setting hardware breakpoint %d at %08X", index, address);

  // Set DRx register and configure DR7
  DebugRegisters regs;
  getDebugRegisters(regs);

  switch (index) {
    case 0: regs.dr0 = address; break;
    case 1: regs.dr1 = address; break;
    case 2: regs.dr2 = address; break;
    case 3: regs.dr3 = address; break;
  }

  // Configure DR7: enable local breakpoint, set type and size
  uint32_t dr7_bits = (1 << (index * 2)); // Local enable
  dr7_bits |= (type << (16 + index * 4));  // Type
  dr7_bits |= (size << (18 + index * 4));  // Size
  regs.dr7 |= dr7_bits;

  return setDebugRegisters(regs);
}

ErrorCode Debug::clearHardwareBreakpoint(uint8_t index) {
  if (index > 3) return kErrorInvalidArgument;

  DebugRegisters regs;
  getDebugRegisters(regs);

  // Clear enable bit in DR7
  regs.dr7 &= ~(3 << (index * 2));

  return setDebugRegisters(regs);
}

ErrorCode Debug::getDebugRegisters(DebugRegisters &regs) {
  memset(&regs, 0, sizeof(regs));
  // Access debug registers via DPMI or direct access in ring 0
  return kSuccess;
}

ErrorCode Debug::setDebugRegisters(const DebugRegisters &regs) {
  // Set debug registers via DPMI or direct access in ring 0
  return kSuccess;
}

ErrorCode Debug::readMemory(uint16_t segment, uint16_t offset,
                            void *data, size_t size) {
  // In protected mode, use selector:offset
  uint32_t linear = 0;
  // Convert selector:offset to linear via DPMI
  return readLinearMemory(linear, data, size);
}

ErrorCode Debug::writeMemory(uint16_t segment, uint16_t offset,
                             const void *data, size_t size) {
  uint32_t linear = 0;
  return writeLinearMemory(linear, data, size);
}

ErrorCode Debug::readLinearMemory(uint32_t address, void *data, size_t size) {
  // DPMI function 0800h - Physical address mapping
  // or direct access if page mapped
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeLinearMemory(uint32_t address, const void *data, size_t size) {
  // DPMI function 0800h + write
  return kSuccess;
}

ErrorCode Debug::getRegisters(void *registers, size_t size) {
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(const void *registers, size_t size) {
  return kSuccess;
}

ErrorCode Debug::readPSP(uint16_t psp_segment, PSP &psp) {
  memset(&psp, 0, sizeof(psp));
  return kSuccess;
}

ErrorCode Debug::enumerateMemoryBlocks(std::vector<MCB> &blocks) {
  blocks.clear();
  return kSuccess;
}

ErrorCode Debug::getEnvironment(uint16_t psp_segment, std::string &env) {
  env.clear();
  return kSuccess;
}

ErrorCode Debug::getDPMIHostInfo(void *buffer, size_t size) {
  // DPMI function 0401h - Get DPMI capabilities
  memset(buffer, 0, size);
  return kSuccess;
}

ErrorCode Debug::allocateDOSMemory(uint16_t paragraphs, uint16_t &segment) {
  // DPMI function 0100h - Allocate DOS memory
  segment = 0;
  return kSuccess;
}

ErrorCode Debug::freeDOSMemory(uint16_t segment) {
  // DPMI function 0101h - Free DOS memory
  return kSuccess;
}

ErrorCode Debug::getInterruptVector(uint8_t interrupt, uint32_t &address) {
  // DPMI function 0200h - Get real mode interrupt vector
  // or 0204h for protected mode
  address = 0;
  return kSuccess;
}

ErrorCode Debug::setInterruptVector(uint8_t interrupt, uint32_t address) {
  // DPMI function 0201h or 0205h
  return kSuccess;
}

ErrorCode Debug::getCPUType(uint8_t &cpu_type) {
  cpu_type = 3; // 386+
  return kSuccess;
}

ErrorCode Debug::getFPUType(uint8_t &fpu_type) {
  fpu_type = 3; // 387+
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
