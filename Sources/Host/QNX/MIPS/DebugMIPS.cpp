#include "DebugServer2/Host/QNX/Debug.h"
#include "DebugServer2/Architecture/MIPS/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>
namespace ds2 { namespace Host { namespace QNX {
static std::map<Address, uint32_t> g_bp;
ErrorCode Debug::attachProcess(pid_t pid, uint32_t flags) { return kSuccess; }
ErrorCode Debug::detachProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::killProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::freezeThread(tid_t tid) { return kSuccess; }
ErrorCode Debug::thawThread(tid_t tid) { return kSuccess; }
ErrorCode Debug::runThread(tid_t tid, uint32_t flags) { return kSuccess; }
ErrorCode Debug::setBreakpoint(pid_t pid, Address addr) {
  uint32_t orig; readMemory(pid, addr, &orig, 4); g_bp[addr] = orig;
  uint32_t brk = 0x0000000D; // MIPS BREAK
  return writeMemory(pid, addr, &brk, 4);
}
ErrorCode Debug::clearBreakpoint(pid_t pid, Address addr) {
  auto it = g_bp.find(addr); if (it == g_bp.end()) return kErrorInvalidArgument;
  return writeMemory(pid, addr, &it->second, 4);
}
ErrorCode Debug::setWatchpoint(pid_t pid, Address addr, uint32_t sz, uint32_t type) { return kSuccess; }
ErrorCode Debug::clearWatchpoint(pid_t pid, Address addr) { return kSuccess; }
ErrorCode Debug::readMemory(pid_t pid, Address addr, void *data, size_t size) { memcpy(data, (void*)addr.value(), size); return kSuccess; }
ErrorCode Debug::writeMemory(pid_t pid, Address addr, const void *data, size_t size) { memcpy((void*)addr.value(), data, size); return kSuccess; }
ErrorCode Debug::readRegisters(tid_t tid, void *regs, size_t size) { return kSuccess; }
ErrorCode Debug::writeRegisters(tid_t tid, const void *regs, size_t size) { return kSuccess; }
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
}}}
