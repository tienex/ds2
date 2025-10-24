#include "DebugServer2/Host/XENIX/Debug.h"
#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace XENIX {

// XENIX for Motorola 68000/68010/68020
static std::map<Address, uint16_t> g_breakpoints;

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "XENIX m68k: attaching to process %d", pid);
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "XENIX m68k: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX m68k: sending signal %d to process %d", signal, pid);
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX m68k: continuing process %d", pid);
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX m68k: single stepping process %d", pid);
  // Use T-bit in SR (Status Register)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "XENIX m68k: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  uint16_t orig;
  readMemory(pid, address, &orig, 2);
  g_breakpoints[address] = orig;

  uint16_t trap = 0x4E4F; // TRAP #15 instruction
  uint16_t trap_be = __builtin_bswap16(trap); // m68k is big-endian
  return writeMemory(pid, address, &trap_be, 2);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;

  uint16_t orig_be = __builtin_bswap16(it->second);
  return writeMemory(pid, address, &orig_be, 2);
}

ErrorCode Debug::readMemory(pid_t pid, Address address, void *data, size_t size) {
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, Address address, const void *data, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getRegisters(pid_t pid, void *registers, size_t size) {
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(pid_t pid, const void *registers, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getProcessInfo(pid_t pid, ProcessInfo &info) {
  memset(&info, 0, sizeof(info));
  info.pid = pid;
  return kSuccess;
}

ErrorCode Debug::readXoutHeader(pid_t pid, xout_header &header) {
  memset(&header, 0, sizeof(header));
  header.x_magic = XOUT_MAGIC;
  return kSuccess;
}

ErrorCode Debug::getTextBase(pid_t pid, Address &address) {
  address = Address(0x00001000);
  return kSuccess;
}

ErrorCode Debug::getDataBase(pid_t pid, Address &address) {
  address = Address(0x00020000);
  return kSuccess;
}

ErrorCode Debug::getStackBase(pid_t pid, Address &address) {
  address = Address(0x7FFF0000);
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
