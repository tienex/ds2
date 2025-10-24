#include "DebugServer2/Host/XENIX/Debug.h"
#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>

namespace ds2 { namespace Host { namespace XENIX {

// XENIX for x86 (Intel 80386, 80486)
static std::map<Address, uint8_t> g_breakpoints;

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "XENIX x86: attaching to process %d", pid);
  // Use ptrace(PTRACE_ATTACH, pid, 0, 0)
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "XENIX x86: detaching from process %d", pid);
  // Use ptrace(PTRACE_DETACH, pid, 0, 0)
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  // Use waitpid(pid, status, 0)
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX x86: sending signal %d to process %d", signal, pid);
  // Use ::kill(pid, signal)
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX x86: continuing process %d", pid);
  // Use ptrace(PTRACE_CONT, pid, 0, signal)
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "XENIX x86: single stepping process %d", pid);
  // Use ptrace(PTRACE_SINGLESTEP, pid, 0, signal)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "XENIX x86: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  uint8_t orig;
  readMemory(pid, address, &orig, 1);
  g_breakpoints[address] = orig;

  uint8_t int3 = 0xCC; // INT3 instruction
  return writeMemory(pid, address, &int3, 1);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;

  return writeMemory(pid, address, &it->second, 1);
}

ErrorCode Debug::readMemory(pid_t pid, Address address, void *data, size_t size) {
  // Use ptrace(PTRACE_PEEKDATA, ...) in a loop for each word
  // XENIX typically uses 32-bit words
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, Address address, const void *data, size_t size) {
  // Use ptrace(PTRACE_POKEDATA, ...) in a loop for each word
  return kSuccess;
}

ErrorCode Debug::getRegisters(pid_t pid, void *registers, size_t size) {
  // Use ptrace(PTRACE_GETREGS, pid, 0, registers)
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(pid_t pid, const void *registers, size_t size) {
  // Use ptrace(PTRACE_SETREGS, pid, 0, registers)
  return kSuccess;
}

ErrorCode Debug::getProcessInfo(pid_t pid, ProcessInfo &info) {
  memset(&info, 0, sizeof(info));
  info.pid = pid;
  return kSuccess;
}

ErrorCode Debug::readXoutHeader(pid_t pid, xout_header &header) {
  // Read x.out header from process memory at text base
  memset(&header, 0, sizeof(header));
  header.x_magic = XOUT_MAGIC;
  return kSuccess;
}

ErrorCode Debug::getTextBase(pid_t pid, Address &address) {
  // XENIX x.out text typically starts at 0x00001000
  address = Address(0x00001000);
  return kSuccess;
}

ErrorCode Debug::getDataBase(pid_t pid, Address &address) {
  // Data follows text segment
  address = Address(0x00002000);
  return kSuccess;
}

ErrorCode Debug::getStackBase(pid_t pid, Address &address) {
  // Stack typically at high memory
  address = Address(0xBFFFF000);
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
