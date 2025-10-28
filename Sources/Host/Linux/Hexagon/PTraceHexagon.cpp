#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Architecture/Hexagon/CPUState.h"
#include "DebugServer2/Host/Platform.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>
using ds2::Host::Linux::PTrace;
using ds2::Host::Platform;
#define super ds2::Host::POSIX::PTrace
namespace ds2 { namespace Host { namespace Linux {
ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));
  user_regs_struct regs;
  struct iovec iov = { &regs, sizeof(regs) };
  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();
  for (int i = 0; i < 32; i++) state.gp.r[i] = regs.r[i];
  state.pc = regs.pc; state.usr = regs.usr;
  return kSuccess;
}
ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));
  user_regs_struct regs;
  for (int i = 0; i < 32; i++) regs.r[i] = state.gp.r[i];
  regs.pc = state.pc; regs.usr = state.usr;
  struct iovec iov = { &regs, sizeof(regs) };
  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();
  return kSuccess;
}
}}}
