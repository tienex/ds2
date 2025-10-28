#include "DebugServer2/Host/NeXT/Debug.h"
#include "DebugServer2/Architecture/M68k/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace NeXT {

// NeXTSTEP/OpenStep for m68k (NeXT Computer, 68030/68040)
static std::map<Address, uint16_t> g_breakpoints;

ErrorCode Debug::taskForPid(pid_t pid, task_port_t &task) {
  DS2LOG(Debug, "NeXTSTEP m68k: getting task for PID %d", pid);
  task = pid; // Simplified mapping
  return kSuccess;
}

ErrorCode Debug::suspendTask(task_port_t task) {
  DS2LOG(Debug, "NeXTSTEP m68k: suspending task %u", task);
  // Use task_suspend() Mach call
  return kSuccess;
}

ErrorCode Debug::resumeTask(task_port_t task) {
  DS2LOG(Debug, "NeXTSTEP m68k: resuming task %u", task);
  // Use task_resume() Mach call
  return kSuccess;
}

ErrorCode Debug::terminateTask(task_port_t task) {
  DS2LOG(Debug, "NeXTSTEP m68k: terminating task %u", task);
  // Use task_terminate() Mach call
  return kSuccess;
}

ErrorCode Debug::getThreads(task_port_t task, std::vector<thread_port_t> &threads) {
  DS2LOG(Debug, "NeXTSTEP m68k: getting threads for task %u", task);
  threads.clear();
  // Use task_threads() Mach call
  return kSuccess;
}

ErrorCode Debug::suspendThread(thread_port_t thread) {
  // Use thread_suspend() Mach call
  return kSuccess;
}

ErrorCode Debug::resumeThread(thread_port_t thread) {
  // Use thread_resume() Mach call
  return kSuccess;
}

ErrorCode Debug::abortThread(thread_port_t thread) {
  // Use thread_abort() Mach call
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(task_port_t task, Address address) {
  DS2LOG(Debug, "NeXTSTEP m68k: setting breakpoint at 0x%08llx",
         (unsigned long long)address.value());
  uint16_t orig;
  readMemory(task, address, &orig, 2);
  g_breakpoints[address] = orig;
  uint16_t trap = 0x4E4F; // TRAP #15
  return writeMemory(task, address, &trap, 2);
}

ErrorCode Debug::clearBreakpoint(task_port_t task, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;
  return writeMemory(task, address, &it->second, 2);
}

ErrorCode Debug::setWatchpoint(task_port_t task, Address address, uint32_t size, uint32_t type) {
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(task_port_t task, Address address) {
  return kSuccess;
}

ErrorCode Debug::readMemory(task_port_t task, Address address, void *data, size_t size) {
  // Use vm_read() Mach call
  memcpy(data, (void*)address.value(), size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(task_port_t task, Address address, const void *data, size_t size) {
  // Use vm_write() Mach call
  memcpy((void*)address.value(), data, size);
  return kSuccess;
}

ErrorCode Debug::protectMemory(task_port_t task, Address address, size_t size, uint32_t protection) {
  // Use vm_protect() Mach call
  return kSuccess;
}

ErrorCode Debug::getThreadState(thread_port_t thread, void *state, size_t &size) {
  // Use thread_get_state() with M68K_THREAD_STATE_REGS
  return kSuccess;
}

ErrorCode Debug::setThreadState(thread_port_t thread, const void *state, size_t size) {
  // Use thread_set_state()
  return kSuccess;
}

ErrorCode Debug::getTaskInfo(task_port_t task, task_basic_info &info) {
  memset(&info, 0, sizeof(info));
  return kSuccess;
}

ErrorCode Debug::getThreadInfo(thread_port_t thread, thread_basic_info &info) {
  memset(&info, 0, sizeof(info));
  return kSuccess;
}

ErrorCode Debug::getVMRegions(task_port_t task, std::vector<vm_region_info> &regions) {
  regions.clear();
  // Use vm_region() Mach call
  return kSuccess;
}

ErrorCode Debug::catchExceptions(task_port_t task) {
  // Use task_set_exception_ports()
  return kSuccess;
}

ErrorCode Debug::waitForException(task_port_t task, debug_exception &exception, uint32_t timeout_ms) {
  memset(&exception, 0, sizeof(exception));
  // Use mach_msg() to receive exception messages
  return kSuccess;
}

ErrorCode Debug::deallocatePort(mach_port_t port) {
  // Use mach_port_deallocate()
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
