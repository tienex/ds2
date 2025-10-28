#include "DebugServer2/Host/Plan9/Debug.h"
#include "DebugServer2/Utils/Log.h"
#include <cstring>
#include <map>
namespace ds2 { namespace Host { namespace Plan9 {
static std::map<Address, uint32_t> g_bp;
ErrorCode Debug::attachProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::detachProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::killProcess(pid_t pid) { return kSuccess; }
ErrorCode Debug::hangProcess(pid_t pid) { return writeProcFile(pid, "ctl", CTL_HANG); }
ErrorCode Debug::unhangProcess(pid_t pid) { return writeProcFile(pid, "ctl", CTL_UNHANG); }
ErrorCode Debug::stepProcess(pid_t pid) { return writeProcFile(pid, "ctl", CTL_STEP); }
ErrorCode Debug::startProcess(pid_t pid) { return writeProcFile(pid, "ctl", CTL_START); }
ErrorCode Debug::waitStop(pid_t pid) { return writeProcFile(pid, "ctl", CTL_WAITSTOP); }
ErrorCode Debug::setBreakpoint(pid_t pid, Address addr) { return kSuccess; }
ErrorCode Debug::clearBreakpoint(pid_t pid, Address addr) { return kSuccess; }
ErrorCode Debug::readMemory(pid_t pid, Address addr, void *data, size_t size) { memcpy(data,(void*)addr.value(),size); return kSuccess; }
ErrorCode Debug::writeMemory(pid_t pid, Address addr, const void *data, size_t size) { memcpy((void*)addr.value(),data,size); return kSuccess; }
ErrorCode Debug::readRegisters(pid_t pid, void *regs, size_t size) { return kSuccess; }
ErrorCode Debug::writeRegisters(pid_t pid, const void *regs, size_t size) { return kSuccess; }
ErrorCode Debug::readFPRegisters(pid_t pid, void *fpregs, size_t size) { return kSuccess; }
ErrorCode Debug::writeFPRegisters(pid_t pid, const void *fpregs, size_t size) { return kSuccess; }
ErrorCode Debug::getStatus(pid_t pid, proc_status &status) { memset(&status,0,sizeof(status)); status.pid=pid; return kSuccess; }
ErrorCode Debug::getSegments(pid_t pid, std::vector<proc_segment> &segments) { segments.clear(); return kSuccess; }
ErrorCode Debug::getFileDescriptors(pid_t pid, std::vector<proc_fd> &fds) { fds.clear(); return kSuccess; }
ErrorCode Debug::waitForProcess(pid_t pid, proc_wait &wait) { memset(&wait,0,sizeof(wait)); wait.pid=pid; return kSuccess; }
ErrorCode Debug::getProcessList(std::vector<pid_t> &pids) { pids.clear(); return kSuccess; }
ErrorCode Debug::readProcFile(pid_t pid, const char *filename, std::string &content) { content.clear(); return kSuccess; }
ErrorCode Debug::writeProcFile(pid_t pid, const char *filename, const std::string &content) { return kSuccess; }
ErrorCode Debug::parseRegisterFile(const std::string &content, void *regs, size_t size) { return kSuccess; }
ErrorCode Debug::formatRegisterFile(const void *regs, size_t size, std::string &content) { content.clear(); return kSuccess; }
bool Debug::_initialized = false;
}}}
