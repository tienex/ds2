#include "DebugServer2/Host/MINIX/Debug.h"
#include "DebugServer2/Architecture/ARM/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace MINIX {

// MINIX 3 for ARM32 (BeagleBone, etc.)
static std::map<Address, uint32_t> g_breakpoints;
MinixVersion Debug::_version = MINIX_V3;

ErrorCode Debug::getMinixVersion(MinixVersion &version) {
  version = _version;
  return kSuccess;
}

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "MINIX ARM32: attaching to process %d", pid);
  // MINIX 3 ptrace(T_ATTACH, ...)
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "MINIX ARM32: detaching from process %d", pid);
  // MINIX 3 ptrace(T_DETACH, ...)
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  // waitpid()
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "MINIX ARM32: sending signal %d to process %d", signal, pid);
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "MINIX ARM32: continuing process %d", pid);
  // ptrace(T_RESUME, ...)
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "MINIX ARM32: single stepping process %d", pid);
  // ptrace(T_STEP, ...)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "MINIX ARM32: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  uint32_t orig;
  readMemory(pid, address, &orig, 4);
  g_breakpoints[address] = orig;

  // ARM32 breakpoint instruction (BKPT #0 for ARM mode, or UDF for Thumb)
  uint32_t bkpt = 0xE1200070; // BKPT #0 in ARM mode
  return writeMemory(pid, address, &bkpt, 4);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;
  return writeMemory(pid, address, &it->second, 4);
}

ErrorCode Debug::readMemory(pid_t pid, Address address, void *data, size_t size) {
  // ptrace(T_GETDATA, ...) in a loop
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, Address address, const void *data, size_t size) {
  // ptrace(T_SETDATA, ...) in a loop
  return kSuccess;
}

ErrorCode Debug::getRegisters(pid_t pid, void *registers, size_t size) {
  // ptrace(T_GETUSER, ...) to read user area with registers
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(pid_t pid, const void *registers, size_t size) {
  // ptrace(T_SETUSER, ...)
  return kSuccess;
}

ErrorCode Debug::getProcessInfo(pid_t pid, ProcessInfo &info) {
  memset(&info, 0, sizeof(info));
  info.pid = pid;
  return kSuccess;
}

ErrorCode Debug::getProcessState(pid_t pid, ProcessState &state) {
  // Read from /proc/<pid>/psinfo or use ptrace
  state = RUNNABLE;
  return kSuccess;
}

ErrorCode Debug::getMessage(pid_t pid, Message &message) {
  // MINIX 3 message passing inspection
  // Read from process memory or use special ptrace call
  memset(&message, 0, sizeof(message));
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
