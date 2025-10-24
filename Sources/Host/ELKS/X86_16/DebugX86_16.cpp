#include "DebugServer2/Host/ELKS/Debug.h"
#include "DebugServer2/Architecture/X86_16/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace ELKS {

// ELKS for 8086/80286 (16-bit)
static std::map<Address, uint8_t> g_breakpoints;

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "ELKS: attaching to process %d", pid);
  // Use ELKS-specific ptrace system call
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "ELKS: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::wait(pid_t pid, int *status) {
  // ELKS wait() system call
  return kSuccess;
}

ErrorCode Debug::kill(pid_t pid, int signal) {
  DS2LOG(Debug, "ELKS: sending signal %d to process %d", signal, pid);
  // ELKS kill() system call
  return kSuccess;
}

ErrorCode Debug::cont(pid_t pid, int signal) {
  DS2LOG(Debug, "ELKS: continuing process %d", pid);
  // ptrace(PTRACE_CONT, pid, 0, signal)
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, int signal) {
  DS2LOG(Debug, "ELKS: single stepping process %d", pid);
  // Use TF (Trap Flag) in FLAGS register
  // ptrace(PTRACE_SINGLESTEP, pid, 0, signal)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "ELKS: setting breakpoint at 0x%08llx",
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
  // ELKS uses segmented 16-bit addressing
  // ptrace(PTRACE_PEEKDATA, ...) with segment:offset addressing
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, Address address, const void *data, size_t size) {
  // ptrace(PTRACE_POKEDATA, ...) with segment:offset addressing
  return kSuccess;
}

ErrorCode Debug::getRegisters(pid_t pid, void *registers, size_t size) {
  // Read 8086/80286 registers via ptrace
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

ErrorCode Debug::readAout16Header(pid_t pid, aout16_header &header) {
  // Read a.out16 header from process image
  memset(&header, 0, sizeof(header));
  header.a_magic[0] = 0x01;
  header.a_magic[1] = 0x07;  // OMAGIC
  header.a_cpu = 0;  // 8086
  return kSuccess;
}

ErrorCode Debug::getSegmentInfo(pid_t pid, MemorySegment &text,
                                MemorySegment &data, MemorySegment &stack) {
  // ELKS uses near model by default (64KB total)
  // Text segment
  text.segment = 0;
  text.offset = 0;
  text.size = 0;
  text.flags = 0x05; // Read + Execute

  // Data segment (follows text)
  data.segment = 0;
  data.offset = 0;
  data.size = 0;
  data.flags = 0x06; // Read + Write

  // Stack segment (top of memory)
  stack.segment = 0;
  stack.offset = 0xFFF0;
  stack.size = 0x1000;
  stack.flags = 0x06; // Read + Write

  return kSuccess;
}

bool Debug::_initialized = false;

}}}
