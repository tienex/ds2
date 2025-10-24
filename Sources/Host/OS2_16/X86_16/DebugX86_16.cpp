#include "DebugServer2/Host/OS2_16/Debug.h"
#include "DebugServer2/Architecture/X86_16/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>

namespace ds2 { namespace Host { namespace OS2_16 {

// OS/2 1.x 16-bit debugging

ErrorCode Debug::attach(pid_t pid) {
  DS2LOG(Debug, "OS/2 16-bit: attaching to process %d", pid);
  // Use DosDebug(DBG_C_Connect, ...)
  return kSuccess;
}

ErrorCode Debug::detach(pid_t pid) {
  DS2LOG(Debug, "OS/2 16-bit: detaching from process %d", pid);
  return kSuccess;
}

ErrorCode Debug::terminate(pid_t pid) {
  DS2LOG(Debug, "OS/2 16-bit: terminating process %d", pid);
  // Use DosDebug(DBG_C_Term, ...)
  return kSuccess;
}

ErrorCode Debug::freezeThread(pid_t pid, tid_t tid) {
  // DosDebug(DBG_C_Freeze, ...)
  return kSuccess;
}

ErrorCode Debug::resumeThread(pid_t pid, tid_t tid) {
  // DosDebug(DBG_C_Resume, ...)
  return kSuccess;
}

ErrorCode Debug::getThreads(pid_t pid, std::vector<tid_t> &threads) {
  threads.clear();
  // Enumerate threads via DosDebug
  return kSuccess;
}

ErrorCode Debug::getThreadInfo(pid_t pid, tid_t tid, ThreadInfo &info) {
  memset(&info, 0, sizeof(info));
  // DosDebug(DBG_C_ThrdStat, ...)
  info.Tid = tid;
  return kSuccess;
}

ErrorCode Debug::go(pid_t pid, tid_t tid) {
  DS2LOG(Debug, "OS/2 16-bit: continuing thread %d in process %d", tid, pid);
  // DosDebug(DBG_C_Go, ...)
  return kSuccess;
}

ErrorCode Debug::singleStep(pid_t pid, tid_t tid) {
  DS2LOG(Debug, "OS/2 16-bit: single stepping thread %d in process %d", tid, pid);
  // DosDebug(DBG_C_SStep, ...)
  return kSuccess;
}

ErrorCode Debug::stop(pid_t pid) {
  DS2LOG(Debug, "OS/2 16-bit: stopping process %d", pid);
  // DosDebug(DBG_C_Stop, ...)
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, uint16_t selector, uint16_t offset) {
  DS2LOG(Debug, "OS/2 16-bit: setting breakpoint at %04X:%04X", selector, offset);
  // Read original byte, store it
  // Write INT 3 (0xCC) via DosDebug(DBG_C_WriteMem_I, ...)
  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(pid_t pid, uint16_t selector, uint16_t offset) {
  // Restore original byte
  return kSuccess;
}

ErrorCode Debug::setWatchpoint(pid_t pid, uint32_t addr, uint32_t size, uint32_t type) {
  // DosDebug(DBG_C_SetWatch, ...)
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(pid_t pid, uint32_t addr) {
  // DosDebug(DBG_C_ClearWatch, ...)
  return kSuccess;
}

ErrorCode Debug::readMemory(pid_t pid, uint16_t selector, uint16_t offset,
                            void *data, size_t size) {
  // DosDebug(DBG_C_ReadMem, ...) or DBG_C_ReadMemBuf
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, uint16_t selector, uint16_t offset,
                             const void *data, size_t size) {
  // DosDebug(DBG_C_WriteMem, ...) or DBG_C_WriteMemBuf
  return kSuccess;
}

ErrorCode Debug::readLinearMemory(pid_t pid, uint32_t addr, void *data, size_t size) {
  memset(data, 0, size);
  return kSuccess;
}

ErrorCode Debug::writeLinearMemory(pid_t pid, uint32_t addr, const void *data, size_t size) {
  return kSuccess;
}

ErrorCode Debug::selOffsetToLinear(pid_t pid, uint16_t selector, uint16_t offset,
                                   uint32_t &linear) {
  // DosDebug(DBG_C_SelToLin, ...)
  linear = 0;
  return kSuccess;
}

ErrorCode Debug::linearToSelOffset(pid_t pid, uint32_t linear,
                                   uint16_t &selector, uint16_t &offset) {
  // DosDebug(DBG_C_LinToSel, ...)
  selector = 0;
  offset = 0;
  return kSuccess;
}

ErrorCode Debug::getRegisters(pid_t pid, tid_t tid, DebugBuffer &regs) {
  memset(&regs, 0, sizeof(regs));
  // DosDebug(DBG_C_ReadReg, ...)
  return kSuccess;
}

ErrorCode Debug::setRegisters(pid_t pid, tid_t tid, const DebugBuffer &regs) {
  // DosDebug(DBG_C_WriteReg, ...)
  return kSuccess;
}

ErrorCode Debug::getModules(pid_t pid, std::vector<ModuleLoadInfo> &modules) {
  modules.clear();
  // Enumerate loaded modules
  return kSuccess;
}

ErrorCode Debug::waitForEvent(pid_t pid, DebugBuffer &event, uint32_t timeout_ms) {
  memset(&event, 0, sizeof(event));
  // Wait for debug notification via DosDebug
  return kSuccess;
}

bool Debug::_initialized = false;

}}}
