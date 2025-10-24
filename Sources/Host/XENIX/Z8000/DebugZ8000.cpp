#include "DebugServer2/Host/XENIX/Debug.h"
#include "DebugServer2/Architecture/Z8000/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace XENIX {

// XENIX for Zilog Z8000
static std::map<Address, uint16_t> g_breakpoints;

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "XENIX Z8000: attaching to process %d", pid);
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "XENIX Z8000: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX Z8000: sending signal %d to process %d", signal, pid);
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX Z8000: continuing process %d", pid);
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX Z8000: single stepping process %d", pid);
  // Z8000 has trace mode via FCW
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "XENIX Z8000: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  uint16_t orig;
  readMemory(pid, address, &orig, 2);
  g_breakpoints[address] = orig;

  // Z8000 breakpoint: SC (System Call) instruction or trap
  uint16_t sc = 0x7F00; // SC (System Call) - exact encoding varies
  return writeMemory(pid, address, &sc, 2);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;
  return writeMemory(pid, address, &it->second, 2);
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
  header.x_cpu = 0x8000; // Z8000 CPU type
  return kSuccess;
}

ErrorCode Debug::getTextBase(pid_t pid, Address &address) {
  address = Address(0x0000);
  return kSuccess;
}

ErrorCode Debug::getDataBase(pid_t pid, Address &address) {
  address = Address(0x1000);
  return kSuccess;
}

ErrorCode Debug::getStackBase(pid_t pid, Address &address) {
  address = Address(0xF000);
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
