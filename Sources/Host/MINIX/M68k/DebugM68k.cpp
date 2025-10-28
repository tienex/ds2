#include "DebugServer2/Host/MINIX/Debug.h"
#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace MINIX {

static std::map<Address, uint16_t> g_breakpoints;
MinixVersion Debug::_version = MINIX_V2;

ErrorCode Debug::getMinixVersion(MinixVersion &version) {
  version = _version;
  return kSuccess;
}

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "MINIX m68k: attaching to process %d", pid);
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "MINIX m68k: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) { return kSuccess; }
ErrorCode Debug::kill(pid_t pid, int signal) { return kSuccess; }
ErrorCode Debug::cont(pid_t pid, int signal) { return kSuccess; }
ErrorCode Debug::singleStep(pid_t pid, int signal) { return kSuccess; }

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  uint16_t orig;
  readMemory(pid, address, &orig, 2);
  g_breakpoints[address] = orig;
  uint16_t trap = 0x4E4F;
  uint16_t trap_be = __builtin_bswap16(trap);
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

ErrorCode Debug::getProcessState(pid_t pid, ProcessState &state) {
  state = RUNNABLE;
  return kSuccess;
}

ErrorCode Debug::getMessage(pid_t pid, Message &message) {
  memset(&message, 0, sizeof(message));
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
