#include "DebugServer2/Host/DOS/Debug.h"
#include "DebugServer2/Architecture/X86_16/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace DOS {

// DOS Real Mode debugging (8086/8088/80286)
static std::map<uint32_t, uint8_t> g_breakpoints;
DOSMode Debug::_current_mode = DOS_REAL_MODE;

ErrorCode Debug::detectMode(DOSMode &mode) {
  // Check CPU type and current mode
  // INT 15h, AX=C0h for system configuration
  mode = _current_mode;
  return kSuccess;
}

bool Debug::isDPMIAvailable() {
  // INT 2Fh, AX=1687h - DPMI installation check
  return false; // Real mode DOS
}

ErrorCode Debug::getDPMIVersion(DPMIVersion &version) {
  memset(&version, 0, sizeof(version));
  return kErrorNotSupported;
}

ErrorCode Debug::loadProgram(const char *path, uint16_t &psp_segment) {
  DS2LOG(Debug, "DOS: loading program %s", path);
  // INT 21h, AH=4Bh - EXEC
  psp_segment = 0;
  return kSuccess;
}

ErrorCode Debug::terminateProgram(uint16_t psp_segment) {
  DS2LOG(Debug, "DOS: terminating program at PSP %04X", psp_segment);
  // INT 21h, AH=4Ch - Exit
  return kSuccess;
}

ErrorCode Debug::run() {
  // Clear trap flag, continue execution
  return kSuccess;
}

ErrorCode Debug::singleStep() {
  // Set trap flag (TF) in FLAGS, execute one instruction
  // INT 1 will fire after instruction
  return kSuccess;
}

ErrorCode Debug::runUntil(uint16_t segment, uint16_t offset) {
  // Set temporary breakpoint at target, run
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(uint16_t segment, uint16_t offset) {
  uint32_t linear = (static_cast<uint32_t>(segment) << 4) + offset;
  DS2LOG(Debug, "DOS: setting breakpoint at %04X:%04X (linear %08X)",
         segment, offset, linear);

  uint8_t orig;
  readMemory(segment, offset, &orig, 1);
  g_breakpoints[linear] = orig;

  uint8_t int3 = 0xCC; // INT 3
  return writeMemory(segment, offset, &int3, 1);
}

ErrorCode Debug::clearBreakpoint(uint16_t segment, uint16_t offset) {
  uint32_t linear = (static_cast<uint32_t>(segment) << 4) + offset;
  auto it = g_breakpoints.find(linear);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;

  return writeMemory(segment, offset, &it->second, 1);
}

ErrorCode Debug::setHardwareBreakpoint(uint8_t index, uint32_t address,
                                       WatchpointType type, WatchpointSize size) {
  // Not available in real mode
  return kErrorNotSupported;
}

ErrorCode Debug::clearHardwareBreakpoint(uint8_t index) {
  return kErrorNotSupported;
}

ErrorCode Debug::getDebugRegisters(DebugRegisters &regs) {
  memset(&regs, 0, sizeof(regs));
  return kErrorNotSupported;
}

ErrorCode Debug::setDebugRegisters(const DebugRegisters &regs) {
  return kErrorNotSupported;
}

ErrorCode Debug::readMemory(uint16_t segment, uint16_t offset,
                            void *data, size_t size) {
  // Direct memory access in real mode
  uint32_t linear = (static_cast<uint32_t>(segment) << 4) + offset;
  memset(data, 0, size); // Stub
  return kSuccess;
}

ErrorCode Debug::writeMemory(uint16_t segment, uint16_t offset,
                             const void *data, size_t size) {
  // Direct memory access in real mode
  return kSuccess;
}

ErrorCode Debug::readLinearMemory(uint32_t address, void *data, size_t size) {
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeLinearMemory(uint32_t address, const void *data, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getRegisters(void *registers, size_t size) {
  memset(registers, 0, size);
  // Read from INT 1 trap frame
  return kSuccess;
}

ErrorCode Debug::setRegisters(const void *registers, size_t size) {
  // Modify INT 1 trap frame
  return kSuccess;
}

ErrorCode Debug::readPSP(uint16_t psp_segment, PSP &psp) {
  memset(&psp, 0, sizeof(psp));
  // Read PSP from segment
  return readMemory(psp_segment, 0, &psp, sizeof(psp));
}

ErrorCode Debug::enumerateMemoryBlocks(std::vector<MCB> &blocks) {
  blocks.clear();
  // Walk MCB chain starting from first MCB
  // INT 21h, AH=52h - Get list of lists
  return kSuccess;
}

ErrorCode Debug::getEnvironment(uint16_t psp_segment, std::string &env) {
  PSP psp;
  ErrorCode error = readPSP(psp_segment, psp);
  if (error != kSuccess) return error;

  // Read environment from psp.environment_seg
  env.clear();
  return kSuccess;
}

ErrorCode Debug::getDPMIHostInfo(void *buffer, size_t size) {
  return kErrorNotSupported;
}

ErrorCode Debug::allocateDOSMemory(uint16_t paragraphs, uint16_t &segment) {
  // INT 21h, AH=48h - Allocate memory
  segment = 0;
  return kSuccess;
}

ErrorCode Debug::freeDOSMemory(uint16_t segment) {
  // INT 21h, AH=49h - Free memory
  return kSuccess;
}

ErrorCode Debug::getInterruptVector(uint8_t interrupt, uint32_t &address) {
  // INT 21h, AH=35h - Get interrupt vector
  address = 0;
  return kSuccess;
}

ErrorCode Debug::setInterruptVector(uint8_t interrupt, uint32_t address) {
  // INT 21h, AH=25h - Set interrupt vector
  return kSuccess;
}

ErrorCode Debug::getCPUType(uint8_t &cpu_type) {
  // Detect CPU: 8086, 8088, 80186, 80286
  cpu_type = 0; // 8086
  return kSuccess;
}

ErrorCode Debug::getFPUType(uint8_t &fpu_type) {
  // Detect FPU: none, 8087, 80287
  fpu_type = 0; // None
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
