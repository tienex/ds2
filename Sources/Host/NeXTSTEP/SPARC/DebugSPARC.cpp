#include "DebugServer2/Host/NeXT/Debug.h"
#include "DebugServer2/Architecture/SPARC/CPUState.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>
namespace ds2 { namespace Host { namespace NeXT {
static std::map<Address, uint32_t> g_bp;
ErrorCode Debug::taskForPid(pid_t pid, task_port_t &task) { task = pid; return kSuccess; }
ErrorCode Debug::suspendTask(task_port_t task) { return kSuccess; }
ErrorCode Debug::resumeTask(task_port_t task) { return kSuccess; }
ErrorCode Debug::terminateTask(task_port_t task) { return kSuccess; }
ErrorCode Debug::getThreads(task_port_t task, std::vector<thread_port_t> &threads) { threads.clear(); return kSuccess; }
ErrorCode Debug::suspendThread(thread_port_t thread) { return kSuccess; }
ErrorCode Debug::resumeThread(thread_port_t thread) { return kSuccess; }
ErrorCode Debug::abortThread(thread_port_t thread) { return kSuccess; }
ErrorCode Debug::setBreakpoint(task_port_t task, Address addr) { uint32_t o; readMemory(task,addr,&o,4); g_bp[addr]=o; uint32_t ta=0x91D02001; return writeMemory(task,addr,&ta,4); }
ErrorCode Debug::clearBreakpoint(task_port_t task, Address addr) { auto it=g_bp.find(addr); if(it==g_bp.end()) return kErrorInvalidArgument; return writeMemory(task,addr,&it->second,4); }
ErrorCode Debug::setWatchpoint(task_port_t task, Address addr, uint32_t sz, uint32_t type) { return kSuccess; }
ErrorCode Debug::clearWatchpoint(task_port_t task, Address addr) { return kSuccess; }
ErrorCode Debug::readMemory(task_port_t task, Address addr, void *data, size_t size) { memcpy(data,(void*)addr.value(),size); return kSuccess; }
ErrorCode Debug::writeMemory(task_port_t task, Address addr, const void *data, size_t size) { memcpy((void*)addr.value(),data,size); return kSuccess; }
ErrorCode Debug::protectMemory(task_port_t task, Address addr, size_t size, uint32_t prot) { return kSuccess; }
ErrorCode Debug::getThreadState(thread_port_t thread, void *state, size_t &size) { return kSuccess; }
ErrorCode Debug::setThreadState(thread_port_t thread, const void *state, size_t size) { return kSuccess; }
ErrorCode Debug::getTaskInfo(task_port_t task, task_basic_info &info) { memset(&info,0,sizeof(info)); return kSuccess; }
ErrorCode Debug::getThreadInfo(thread_port_t thread, thread_basic_info &info) { memset(&info,0,sizeof(info)); return kSuccess; }
ErrorCode Debug::getVMRegions(task_port_t task, std::vector<vm_region_info> &regions) { regions.clear(); return kSuccess; }
ErrorCode Debug::catchExceptions(task_port_t task) { return kSuccess; }
ErrorCode Debug::waitForException(task_port_t task, debug_exception &exception, uint32_t timeout_ms) { memset(&exception,0,sizeof(exception)); return kSuccess; }
ErrorCode Debug::deallocatePort(mach_port_t port) { return kSuccess; }
bool Debug::_initialized = false;
}}}
