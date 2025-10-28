#include "DebugServer2/Host/NetWare/Debug.h"
#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>

namespace ds2 { namespace Host { namespace NetWare {

ErrorCode Debug::getServerVersion(uint32_t &major, uint32_t &minor, uint32_t &revision) {
  // Call GetFileServerVersion
  major = 5;
  minor = 0;
  revision = 0;
  return kSuccess;
}

ErrorCode Debug::getServerName(std::string &name) {
  name = "NETWARE";
  return kSuccess;
}

ErrorCode Debug::getThreads(std::vector<thread_t> &threads) {
  threads.clear();
  // Enumerate threads via kernel API
  return kSuccess;
}

ErrorCode Debug::getThreadInfo(thread_t thread, ThreadInfo &info) {
  memset(&info, 0, sizeof(info));
  info.thread_id = thread;
  return kSuccess;
}

ErrorCode Debug::suspendThread(thread_t thread) {
  DS2LOG(Debug, "NetWare: suspending thread %u", thread);
  return kSuccess;
}

ErrorCode Debug::resumeThread(thread_t thread) {
  DS2LOG(Debug, "NetWare: resuming thread %u", thread);
  return kSuccess;
}

ErrorCode Debug::terminateThread(thread_t thread) {
  DS2LOG(Debug, "NetWare: terminating thread %u", thread);
  return kSuccess;
}

ErrorCode Debug::setThreadPriority(thread_t thread, uint8_t priority) {
  return kSuccess;
}

ErrorCode Debug::getNLMs(std::vector<nlm_handle_t> &nlms) {
  nlms.clear();
  return kSuccess;
}

ErrorCode Debug::getNLMInfo(nlm_handle_t handle, NLMInfo &info) {
  memset(&info, 0, sizeof(info));
  info.handle = handle;
  return kSuccess;
}

ErrorCode Debug::loadNLM(const char *path, nlm_handle_t &handle) {
  DS2LOG(Debug, "NetWare: loading NLM %s", path);
  handle = 0;
  return kSuccess;
}

ErrorCode Debug::unloadNLM(nlm_handle_t handle) {
  DS2LOG(Debug, "NetWare: unloading NLM %u", handle);
  return kSuccess;
}

ErrorCode Debug::continueThread(thread_t thread) {
  return kSuccess;
}

ErrorCode Debug::singleStep(thread_t thread) {
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(uint32_t address) {
  DS2LOG(Debug, "NetWare: setting breakpoint at %08X", address);
  // Write INT 3
  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(uint32_t address) {
  return kSuccess;
}

ErrorCode Debug::setWatchpoint(uint32_t address, uint32_t size, uint32_t type) {
  // Use debug registers
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(uint32_t address) {
  return kSuccess;
}

ErrorCode Debug::readMemory(uint32_t address, void *data, size_t size) {
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(uint32_t address, const void *data, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getRegisters(thread_t thread, void *registers, size_t size) {
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(thread_t thread, const void *registers, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getStackTrace(thread_t thread, std::vector<uint32_t> &frames) {
  frames.clear();
  return kSuccess;
}

ErrorCode Debug::getStackInfo(thread_t thread, uint32_t &stack_base,
                              uint32_t &stack_size, uint32_t &stack_ptr) {
  stack_base = 0;
  stack_size = 0;
  stack_ptr = 0;
  return kSuccess;
}

ErrorCode Debug::getSymbolAddress(nlm_handle_t nlm, const char *symbol, uint32_t &address) {
  address = 0;
  return kSuccess;
}

ErrorCode Debug::getSymbolName(uint32_t address, std::string &name, uint32_t &offset) {
  name.clear();
  offset = 0;
  return kSuccess;
}

ErrorCode Debug::getResourceTags(nlm_handle_t nlm, std::vector<ResourceTag> &tags) {
  tags.clear();
  return kSuccess;
}

ErrorCode Debug::getConsoleScreen(screen_t &screen) {
  screen = 0;
  return kSuccess;
}

ErrorCode Debug::createScreen(const char *name, screen_t &screen) {
  screen = 0;
  return kSuccess;
}

ErrorCode Debug::destroyScreen(screen_t screen) {
  return kSuccess;
}

ErrorCode Debug::waitForEvent(DebugEvent &event, uint32_t timeout_ms) {
  memset(&event, 0, sizeof(event));
  return kSuccess;
}

ErrorCode Debug::setExceptionHandler(ExceptionType type, void *handler) {
  return kSuccess;
}

ErrorCode Debug::clearExceptionHandler(ExceptionType type) {
  return kSuccess;
}

ErrorCode Debug::getCacheStatistics(uint32_t &buffer_count, uint32_t &dirty_buffers) {
  buffer_count = 0;
  dirty_buffers = 0;
  return kSuccess;
}

ErrorCode Debug::getConnectionCount(uint32_t &count) {
  count = 0;
  return kSuccess;
}

ErrorCode Debug::getMemoryStatistics(uint32_t &total, uint32_t &available) {
  total = 0;
  available = 0;
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
