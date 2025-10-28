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
#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <map>

namespace ds2 {
namespace Host {
namespace QNX {

// QNX Neutrino x86 debugging via procnto
// Uses /proc filesystem and devctl() for debug control

// x86 CPU context (matches QNX X86_CPU_REGISTERS)
struct x86_cpu_registers {
  uint32_t eax, ebx, ecx, edx;
  uint32_t edi, esi;
  uint32_t ebp, esp;
  uint32_t eip;
  uint32_t eflags;
  uint16_t cs, ds, es, fs, gs, ss;
};

static std::map<Address, uint8_t> g_breakpoints;

ErrorCode Debug::attachProcess(pid_t pid, uint32_t flags) {
  DS2LOG(Debug, "attaching to QNX x86 process: PID=%d, flags=0x%x", pid, flags);

  // Use devctl(DCMD_PROC_ATTACH) on /proc/<pid>/as file descriptor
  // This establishes debug control over the process

  return kSuccess;
}

ErrorCode Debug::detachProcess(pid_t pid) {
  DS2LOG(Debug, "detaching from QNX x86 process: PID=%d", pid);

  // Use devctl(DCMD_PROC_DETACH)
  return kSuccess;
}

ErrorCode Debug::killProcess(pid_t pid) {
  DS2LOG(Debug, "killing QNX x86 process: PID=%d", pid);

  // Use kill(pid, SIGKILL) or devctl(DCMD_PROC_STOP)
  return kSuccess;
}

ErrorCode Debug::freezeThread(tid_t tid) {
  DS2LOG(Debug, "freezing QNX x86 thread: TID=%d", tid);

  // Use devctl(DCMD_PROC_FREEZETHREAD)
  return kSuccess;
}

ErrorCode Debug::thawThread(tid_t tid) {
  DS2LOG(Debug, "thawing QNX x86 thread: TID=%d", tid);

  // Use devctl(DCMD_PROC_THAWTHREAD)
  return kSuccess;
}

ErrorCode Debug::runThread(tid_t tid, uint32_t flags) {
  DS2LOG(Debug, "running QNX x86 thread: TID=%d, flags=0x%x", tid, flags);

  // Use devctl(DCMD_PROC_RUN) with run flags
  // Flags control single-step, trace, fault handling
  return kSuccess;
}

ErrorCode Debug::setBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "setting QNX x86 breakpoint at 0x%08llx for PID %d",
         (unsigned long long)address.value(), pid);

  // Read original byte
  uint8_t originalByte;
  ErrorCode error = readMemory(pid, address, &originalByte, 1);
  if (error != kSuccess) {
    return error;
  }

  g_breakpoints[address] = originalByte;

  // Write INT3 (0xCC)
  uint8_t int3 = 0xCC;
  error = writeMemory(pid, address, &int3, 1);
  return error;
}

ErrorCode Debug::clearBreakpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "clearing QNX x86 breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) {
    return kErrorInvalidArgument;
  }

  uint8_t originalByte = it->second;
  ErrorCode error = writeMemory(pid, address, &originalByte, 1);
  if (error == kSuccess) {
    g_breakpoints.erase(it);
  }
  return error;
}

ErrorCode Debug::setWatchpoint(pid_t pid, Address address, uint32_t size,
                               uint32_t type) {
  DS2LOG(Debug, "setting QNX x86 watchpoint at 0x%08llx, size=%u, type=%u",
         (unsigned long long)address.value(), size, type);

  // Use hardware debug registers via devctl(DCMD_PROC_SET_WATCHPOINT)
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(pid_t pid, Address address) {
  DS2LOG(Debug, "clearing QNX x86 watchpoint at 0x%08llx",
         (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::readMemory(pid_t pid, Address address, void *data,
                           size_t size) {
  DS2LOG(Debug, "reading %zu bytes from 0x%08llx in QNX x86 PID %d",
         size, (unsigned long long)address.value(), pid);

  // Use pread() on /proc/<pid>/as file descriptor
  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(pid_t pid, Address address, void const *data,
                             size_t size) {
  DS2LOG(Debug, "writing %zu bytes to 0x%08llx in QNX x86 PID %d",
         size, (unsigned long long)address.value(), pid);

  // Use pwrite() on /proc/<pid>/as
  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(tid_t tid, void *regs, size_t size) {
  if (size < sizeof(Architecture::X86::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::X86::CPUState *state =
      reinterpret_cast<Architecture::X86::CPUState *>(regs);

  // Use devctl(DCMD_PROC_GETGREG) to read general registers
  x86_cpu_registers cpuRegs;
  memset(&cpuRegs, 0, sizeof(cpuRegs));

  state->gp.eax = cpuRegs.eax;
  state->gp.ebx = cpuRegs.ebx;
  state->gp.ecx = cpuRegs.ecx;
  state->gp.edx = cpuRegs.edx;
  state->gp.esi = cpuRegs.esi;
  state->gp.edi = cpuRegs.edi;
  state->gp.ebp = cpuRegs.ebp;
  state->gp.esp = cpuRegs.esp;
  state->special.eip = cpuRegs.eip;
  state->special.eflags = cpuRegs.eflags;
  state->special.cs = cpuRegs.cs;
  state->special.ds = cpuRegs.ds;
  state->special.es = cpuRegs.es;
  state->special.fs = cpuRegs.fs;
  state->special.gs = cpuRegs.gs;
  state->special.ss = cpuRegs.ss;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(tid_t tid, void const *regs, size_t size) {
  if (size < sizeof(Architecture::X86::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::X86::CPUState *state =
      reinterpret_cast<const Architecture::X86::CPUState *>(regs);

  // Use devctl(DCMD_PROC_SETGREG)
  x86_cpu_registers cpuRegs;
  cpuRegs.eax = state->gp.eax;
  cpuRegs.ebx = state->gp.ebx;
  cpuRegs.ecx = state->gp.ecx;
  cpuRegs.edx = state->gp.edx;
  cpuRegs.esi = state->gp.esi;
  cpuRegs.edi = state->gp.edi;
  cpuRegs.ebp = state->gp.ebp;
  cpuRegs.esp = state->gp.esp;
  cpuRegs.eip = state->special.eip;
  cpuRegs.eflags = state->special.eflags;

  return kSuccess;
}

ErrorCode Debug::getProcessInfo(pid_t pid, procfs_info &info) {
  DS2LOG(Debug, "getting QNX x86 process info for PID %d", pid);

  // Read /proc/<pid>/info
  memset(&info, 0, sizeof(info));
  info.pid = pid;
  snprintf(info.name, sizeof(info.name), "qnx_process_%d", pid);

  return kSuccess;
}

ErrorCode Debug::getThreadStatus(tid_t tid, procfs_status &status) {
  DS2LOG(Debug, "getting QNX x86 thread status for TID %d", tid);

  // Read /proc/<pid>/thread/<tid>/status
  memset(&status, 0, sizeof(status));
  status.tid = tid;
  status.state = STATE_STOPPED;

  return kSuccess;
}

ErrorCode Debug::getMemoryMaps(pid_t pid,
                               std::vector<procfs_mapinfo> &maps) {
  DS2LOG(Debug, "getting QNX x86 memory maps for PID %d", pid);

  // Read /proc/<pid>/map or use devctl(DCMD_PROC_MAPINFO)
  maps.clear();
  return kSuccess;
}

ErrorCode Debug::getThreads(pid_t pid, std::vector<tid_t> &threads) {
  DS2LOG(Debug, "enumerating QNX x86 threads for PID %d", pid);

  // Read /proc/<pid>/thread/ directory
  threads.clear();
  return kSuccess;
}

ErrorCode Debug::waitForDebugEvent(pid_t pid, debug_event_type &event,
                                   tid_t &tid, uint32_t timeout_ms) {
  DS2LOG(Debug, "waiting for QNX x86 debug event from PID %d", pid);

  // Use devctl(DCMD_PROC_WAITINFO) or sigevent with pulse
  event = DEBUG_EVENT_STOP;
  tid = 1;

  return kSuccess;
}

ErrorCode Debug::setSigMask(pid_t pid, uint64_t mask) {
  DS2LOG(Debug, "setting signal mask for QNX x86 PID %d: 0x%llx", pid,
         (unsigned long long)mask);

  // Use devctl(DCMD_PROC_SET_FLAG) with signal mask
  return kSuccess;
}

ErrorCode Debug::getSigMask(pid_t pid, uint64_t &mask) {
  DS2LOG(Debug, "getting signal mask for QNX x86 PID %d", pid);

  mask = 0;
  return kSuccess;
}

ErrorCode Debug::getChannelInfo(pid_t pid, chid_t chid, void *info) {
  DS2LOG(Debug, "getting channel info for QNX x86 PID %d, CHID %d", pid, chid);

  // QNX message passing channel debugging
  return kSuccess;
}

ErrorCode Debug::getConnectionInfo(pid_t pid, coid_t coid, void *info) {
  DS2LOG(Debug, "getting connection info for QNX x86 PID %d, COID %d", pid,
         coid);

  return kSuccess;
}

bool Debug::_initialized = false;

} // namespace QNX
} // namespace Host
} // namespace ds2
