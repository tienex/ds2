//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/QNX/Debug.h"
#include "DebugServer2/Architecture/X86_64/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>

namespace ds2 { namespace Host { namespace QNX {

// QNX Neutrino x86-64 debugging via procnto filesystem
static std::map<Address, uint8_t> g_breakpoints;

ErrorCode Debug::attachProcess(pid_t pid, uint32_t flags) {
  DS2LOG(Debug, "attaching to QNX x86-64 process: PID=%d", pid);
  return kSuccess;
}
ErrorCode Debug::detachProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::killProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::freezeThread(tid_t tid) { return kSuccess; }
ErrorCode Debug::thawThread(tid_t tid) { return kSuccess; }
ErrorCode Debug::runThread(tid_t tid, uint32_t flags) { return kSuccess; }

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  uint8_t orig;
  readMemory(pid, address, &orig, 1);
  g_breakpoints[address] = orig;
  uint8_t int3 = 0xCC;
  return writeMemory(pid, address, &int3, 1);
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) return kErrorInvalidArgument;
  return writeMemory(pid, address, &it->second, 1);
}

ErrorCode Debug::setWatchpoint(pid_t pid, Address addr, uint32_t sz, uint32_t type) { return kSuccess; }
ErrorCode Debug::clearWatchpoint(pid_t pid, Address addr) { return kSuccess; }
ErrorCode Debug::readMemory(pid_t pid, Address addr, void *data, size_t size) {
  memcpy(data, reinterpret_cast<const void *>(addr.value()), size);
  return kSuccess;
}
ErrorCode Debug::writeMemory(pid_t pid, Address addr, void const *data, size_t size) {
  memcpy(reinterpret_cast<void *>(addr.value()), data, size);
  return kSuccess;
}
ErrorCode Debug::readRegisters(tid_t tid, void *regs, size_t size) { return kSuccess; }
ErrorCode Debug::writeRegisters(tid_t tid, void const *regs, size_t size) { return kSuccess; }
ErrorCode Debug::getProcessInfo(pid_t pid, procfs_info &info) { memset(&info, 0, sizeof(info)); info.pid = pid; return kSuccess; }
ErrorCode Debug::getThreadStatus(tid_t tid, procfs_status &status) { memset(&status, 0, sizeof(status)); status.tid = tid; return kSuccess; }
ErrorCode Debug::getMemoryMaps(pid_t pid, std::vector<procfs_mapinfo> &maps) { maps.clear(); return kSuccess; }
ErrorCode Debug::getThreads(pid_t pid, std::vector<tid_t> &threads) { threads.clear(); return kSuccess; }
ErrorCode Debug::waitForDebugEvent(pid_t pid, debug_event_type &event, tid_t &tid, uint32_t timeout_ms) { event = DEBUG_EVENT_STOP; tid = 1; return kSuccess; }
ErrorCode Debug::setSigMask(pid_t pid, uint64_t mask) { return kSuccess; }
ErrorCode Debug::getSigMask(pid_t pid, uint64_t &mask) { mask = 0; return kSuccess; }
ErrorCode Debug::getChannelInfo(pid_t pid, chid_t chid, void *info) { return kSuccess; }
ErrorCode Debug::getConnectionInfo(pid_t pid, coid_t coid, void *info) { return kSuccess; }
bool Debug::_initialized = false;

}}} // namespace ds2::Host::QNX
