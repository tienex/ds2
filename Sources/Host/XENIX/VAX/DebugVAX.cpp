#include "DebugServer2/Host/XENIX/Debug.h"
#include "DebugServer2/Architecture/VAX/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace XENIX {

// XENIX for DEC VAX
static std::map<Address, uint8_t> g_breakpoints;

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "XENIX VAX: attaching to process %d", pid);
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "XENIX VAX: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX VAX: sending signal %d to process %d", signal, pid);
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX VAX: continuing process %d", pid);
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX VAX: single stepping process %d", pid);
  // VAX has trace mode via PSL (Processor Status Longword)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "XENIX VAX: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  uint8_t orig;
  readMemory(pid, address, &orig, 1);
  g_breakpoints[address] = orig;

  uint8_t bpt = 0x03; // BPT instruction
  return writeMemory(pid, address, &bpt, 1);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;
  return writeMemory(pid, address, &it->second, 1);
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
  header.x_cpu = 0x0001; // VAX CPU type
  return kSuccess;
}

ErrorCode Debug::getTextBase(pid_t pid, Address &address) {
  address = Address(0x00000000);
  return kSuccess;
}

ErrorCode Debug::getDataBase(pid_t pid, Address &address) {
  address = Address(0x00020000);
  return kSuccess;
}

ErrorCode Debug::getStackBase(pid_t pid, Address &address) {
  address = Address(0x7FFFFFFF);
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
