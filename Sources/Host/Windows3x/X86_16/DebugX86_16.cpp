#include "DebugServer2/Host/Windows3x/Debug.h"
#include "DebugServer2/Architecture/X86_16/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>

namespace ds2 { namespace Host { namespace Windows3x {

WindowsMode Debug::_current_mode = WIN_ENHANCED_MODE;

ErrorCode Debug::detectMode(WindowsMode &mode) {
  // INT 2Fh, AX=1600h - Windows installation check
  mode = _current_mode;
  return kSuccess;
}

uint16_t Debug::getWindowsVersion() {
  // INT 2Fh, AX=160Ah or GetVersion()
  return 0x030A; // Windows 3.10
}

ErrorCode Debug::getTasks(std::vector<TDB> &tasks) {
  tasks.clear();
  // Use TOOLHELP.DLL TaskFirst/TaskNext
  return kSuccess;
}

ErrorCode Debug::getTaskByHandle(uint16_t handle, TDB &task) {
  memset(&task, 0, sizeof(task));
  // TOOLHELP TaskFindHandle
  return kSuccess;
}

ErrorCode Debug::terminateTask(uint16_t task_handle) {
  DS2LOG(Debug, "Windows 3.x: terminating task %04X", task_handle);
  // TOOLHELP TerminateApp
  return kSuccess;
}

ErrorCode Debug::switchToTask(uint16_t task_handle) {
  // Yield to task
  return kSuccess;
}

ErrorCode Debug::getModules(std::vector<MDB> &modules) {
  modules.clear();
  // TOOLHELP ModuleFirst/ModuleNext
  return kSuccess;
}

ErrorCode Debug::getModuleHandle(const char *name, uint16_t &handle) {
  // GetModuleHandle API
  handle = 0;
  return kSuccess;
}

ErrorCode Debug::enumerateGlobalHeap(std::vector<GlobalEntry> &entries) {
  entries.clear();
  // TOOLHELP GlobalFirst/GlobalNext
  return kSuccess;
}

ErrorCode Debug::enumerateLocalHeap(uint16_t segment, std::vector<LocalEntry> &entries) {
  entries.clear();
  // TOOLHELP LocalFirst/LocalNext
  return kSuccess;
}

ErrorCode Debug::getGlobalEntry(uint16_t handle, GlobalEntry &entry) {
  memset(&entry, 0, sizeof(entry));
  // TOOLHELP GlobalEntryHandle
  return kSuccess;
}

ErrorCode Debug::getStackTrace(uint16_t task, std::vector<StackFrame> &frames) {
  frames.clear();
  // TOOLHELP StackTraceFirst/StackTraceNext
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(uint16_t selector, uint16_t offset) {
  DS2LOG(Debug, "Windows 3.x: setting breakpoint at %04X:%04X", selector, offset);
  // INT 3 or use TOOLHELP interrupt hooks
  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(uint16_t selector, uint16_t offset) {
  return kSuccess;
}

ErrorCode Debug::readMemory(uint16_t selector, uint16_t offset, void *data, size_t size) {
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(uint16_t selector, uint16_t offset, const void *data, size_t size) {
  return kSuccess;
}

ErrorCode Debug::getRegisters(uint16_t task, void *registers, size_t size) {
  memset(registers, 0, size);
  return kSuccess;
}

ErrorCode Debug::setRegisters(uint16_t task, const void *registers, size_t size) {
  return kSuccess;
}

ErrorCode Debug::registerNotify(NotifyCallback callback) {
  // TOOLHELP NotifyRegister
  return kSuccess;
}

ErrorCode Debug::unregisterNotify(NotifyCallback callback) {
  // TOOLHELP NotifyUnregister
  return kSuccess;
}

ErrorCode Debug::registerInterruptHandler(uint8_t interrupt, void *handler) {
  // TOOLHELP InterruptRegister
  return kSuccess;
}

ErrorCode Debug::unregisterInterruptHandler(uint8_t interrupt) {
  // TOOLHELP InterruptUnregister
  return kSuccess;
}

ErrorCode Debug::enableAPITrace(bool enable) {
  return kSuccess;
}

ErrorCode Debug::setAPIBreakpoint(const char *module, const char *function) {
  DS2LOG(Debug, "Windows 3.x: setting API breakpoint on %s!%s", module, function);
  return kSuccess;
}

ErrorCode Debug::getMessageQueue(uint16_t task, std::vector<void*> &messages) {
  messages.clear();
  return kSuccess;
}

ErrorCode Debug::getGDIObjects(uint16_t task, std::vector<uint16_t> &handles) {
  handles.clear();
  return kSuccess;
}

ErrorCode Debug::getUserObjects(uint16_t task, std::vector<uint16_t> &handles) {
  handles.clear();
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
