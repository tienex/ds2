#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Architecture/S390X/CPUState.h"
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
  s390_regs regs;
  struct iovec iov = { &regs, sizeof(regs) };
  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();
  for (int i = 0; i < 16; i++) state.gp.r[i] = regs.gprs[i];
  state.psw.psw_mask = regs.psw.mask; state.psw.psw_addr = regs.psw.addr;
  for (int i = 0; i < 16; i++) state.acr.a[i] = regs.acrs[i];
  s390_fp_regs fpregs;
  iov = { &fpregs, sizeof(fpregs) };
  if (wrapPtrace(PTRACE_GETREGSET, pid, (void *)NT_FPREGSET, &iov) >= 0) {
    for (int i = 0; i < 16; i++) state.fpr.f[i] = fpregs.fprs[i];
    state.fpc = fpregs.fpc;
  }
  return kSuccess;
}
ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));
  s390_regs regs;
  for (int i = 0; i < 16; i++) regs.gprs[i] = state.gp.r[i];
  regs.psw.mask = state.psw.psw_mask; regs.psw.addr = state.psw.psw_addr;
  for (int i = 0; i < 16; i++) regs.acrs[i] = state.acr.a[i];
  struct iovec iov = { &regs, sizeof(regs) };
  if (wrapPtrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
    return Platform::TranslateError();
  return kSuccess;
}
}}}
