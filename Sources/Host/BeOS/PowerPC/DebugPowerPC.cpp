//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/BeOS/Debug.h"
#include "DebugServer2/Architecture/PowerPC/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <map>

namespace ds2 {
namespace Host {
namespace BeOS {

// BeOS R5 PowerPC debugging implementation
// BeOS R5 for PowerPC (BeBox, PowerMac clones, 1995-2000)

// CPU state for BeOS PowerPC
struct cpu_state {
  uint32_t gpr[32];   // General purpose registers
  double fpr[32];     // Floating point registers
  uint32_t pc;        // Program counter (SRR0)
  uint32_t msr;       // Machine state register (SRR1)
  uint32_t cr;        // Condition register
  uint32_t lr;        // Link register
  uint32_t ctr;       // Count register
  uint32_t xer;       // Integer exception register
  uint32_t fpscr;     // FP status and control
};

// Breakpoint table
static std::map<Address, uint32_t> g_breakpoints;

// Thread state cache
static std::map<thread_id, cpu_state> g_threadStates;

ErrorCode Debug::attachTeam(team_id team) {
  DS2LOG(Debug, "attaching to BeOS PowerPC team: %d", team);

  // Use install_team_debugger() system call
  return kSuccess;
}

ErrorCode Debug::detachTeam(team_id team) {
  DS2LOG(Debug, "detaching from BeOS PowerPC team: %d", team);

  // Use remove_team_debugger() system call
  return kSuccess;
}

ErrorCode Debug::killTeam(team_id team) {
  DS2LOG(Debug, "killing BeOS PowerPC team: %d", team);

  // Use kill_team() system call
  return kSuccess;
}

ErrorCode Debug::suspendThread(thread_id thread) {
  DS2LOG(Debug, "suspending BeOS PowerPC thread: %d", thread);

  // Use suspend_thread() system call
  return kSuccess;
}

ErrorCode Debug::resumeThread(thread_id thread) {
  DS2LOG(Debug, "resuming BeOS PowerPC thread: %d", thread);

  // Use resume_thread() system call
  return kSuccess;
}

ErrorCode Debug::singleStepThread(thread_id thread) {
  DS2LOG(Debug, "single-stepping BeOS PowerPC thread: %d", thread);

  // Set single-step enable bit in MSR
  auto it = g_threadStates.find(thread);
  if (it != g_threadStates.end()) {
    it->second.msr |= 0x00000400;  // Set SE bit
  }

  return kSuccess;
}

ErrorCode Debug::setBreakpoint(team_id team, Address address) {
  DS2LOG(Debug, "setting PowerPC breakpoint at 0x%08llx for team %d",
         (unsigned long long)address.value(), team);

  // Read original instruction
  uint32_t originalInstr;
  ErrorCode error = readMemory(team, address, &originalInstr,
                               sizeof(originalInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read original instruction");
    return error;
  }

  // Save original instruction
  g_breakpoints[address] = originalInstr;

  // Write trap instruction: tw 31,r0,r0 (0x7FE00008)
  uint32_t trapInstr = 0x7FE00008;
  error = writeMemory(team, address, &trapInstr, sizeof(trapInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(team_id team, Address address) {
  DS2LOG(Debug, "clearing PowerPC breakpoint at 0x%08llx",
         (unsigned long long)address.value());

  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) {
    return kErrorInvalidArgument;
  }

  // Restore original instruction
  uint32_t originalInstr = it->second;
  ErrorCode error = writeMemory(team, address, &originalInstr,
                               sizeof(originalInstr));
  if (error != kSuccess) {
    return error;
  }

  g_breakpoints.erase(it);
  return kSuccess;
}

ErrorCode Debug::setWatchpoint(team_id team, Address address, uint32_t size,
                               uint32_t type) {
  DS2LOG(Debug, "setting PowerPC watchpoint at 0x%08llx, size=%u, type=%u",
         (unsigned long long)address.value(), size, type);

  // PowerPC supports data address breakpoints (DABR)
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(team_id team, Address address) {
  DS2LOG(Debug, "clearing PowerPC watchpoint at 0x%08llx",
         (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::readMemory(team_id team, Address address, void *data,
                           size_t size) {
  DS2LOG(Debug, "reading %zu bytes from 0x%08llx in PowerPC team %d",
         size, (unsigned long long)address.value(), team);

  // Use debugger nub for memory access
  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(team_id team, Address address, void const *data,
                             size_t size) {
  DS2LOG(Debug, "writing %zu bytes to 0x%08llx in PowerPC team %d",
         size, (unsigned long long)address.value(), team);

  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(thread_id thread, void *regs, size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::PowerPC::CPUState *state =
      reinterpret_cast<Architecture::PowerPC::CPUState *>(regs);

  // Get cached thread state
  auto it = g_threadStates.find(thread);
  if (it == g_threadStates.end()) {
    DS2LOG(Error, "thread state not found for PowerPC thread %d", thread);
    return kErrorInvalidArgument;
  }

  const cpu_state &cpuState = it->second;

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    state->gp.regs[i] = cpuState.gpr[i];
  }

  // Copy special registers
  state->special.pc = cpuState.pc;
  state->special.msr = cpuState.msr;
  state->special.cr = cpuState.cr;
  state->special.lr = cpuState.lr;
  state->special.ctr = cpuState.ctr;
  state->special.xer = cpuState.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    state->fpu.regs[i] = cpuState.fpr[i];
  }
  state->fpu.fpscr = cpuState.fpscr;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(thread_id thread, void const *regs,
                                size_t size) {
  if (size < sizeof(Architecture::PowerPC::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::PowerPC::CPUState *state =
      reinterpret_cast<const Architecture::PowerPC::CPUState *>(regs);

  // Get or create thread state
  cpu_state &cpuState = g_threadStates[thread];

  // Copy GPRs
  for (int i = 0; i < 32; i++) {
    cpuState.gpr[i] = state->gp.regs[i];
  }

  // Copy special registers
  cpuState.pc = state->special.pc;
  cpuState.msr = state->special.msr;
  cpuState.cr = state->special.cr;
  cpuState.lr = state->special.lr;
  cpuState.ctr = state->special.ctr;
  cpuState.xer = state->special.xer;

  // Copy FPRs
  for (int i = 0; i < 32; i++) {
    cpuState.fpr[i] = state->fpu.regs[i];
  }
  cpuState.fpscr = state->fpu.fpscr;

  return kSuccess;
}

ErrorCode Debug::getTeamInfo(team_id team, team_info &info) {
  DS2LOG(Debug, "getting PowerPC team info for team %d", team);

  memset(&info, 0, sizeof(info));
  info.id = team;
  info.thread_count = 0;
  info.image_count = 0;
  info.area_count = 0;
  info.flags = 0;
  snprintf(info.name, sizeof(info.name), "BeOS PowerPC Team %d", team);

  return kSuccess;
}

ErrorCode Debug::getThreadInfo(thread_id thread, thread_info &info) {
  DS2LOG(Debug, "getting PowerPC thread info for thread %d", thread);

  memset(&info, 0, sizeof(info));
  info.id = thread;
  info.team = 0;
  info.state = B_THREAD_RUNNING;
  info.priority = 10;
  snprintf(info.name, sizeof(info.name), "ppc_thread_%d", thread);

  return kSuccess;
}

ErrorCode Debug::getImageInfo(area_id image, image_info &info) {
  DS2LOG(Debug, "getting PowerPC image info for image %d", image);

  memset(&info, 0, sizeof(info));
  info.id = image;
  info.type = B_APP_IMAGE;

  return kSuccess;
}

ErrorCode Debug::getAreaInfo(area_id area, area_info &info) {
  DS2LOG(Debug, "getting PowerPC area info for area %d", area);

  memset(&info, 0, sizeof(info));
  info.id = area;
  info.protection = B_READ_AREA | B_WRITE_AREA;

  return kSuccess;
}

ErrorCode Debug::waitForEvent(team_id team, debug_event &event,
                              uint64_t timeout_us) {
  DS2LOG(Debug, "waiting for PowerPC debug event from team %d", team);

  memset(&event, 0, sizeof(event));
  event.type = B_DEBUGGER_CALL;
  event.team = team;
  event.thread = 0;

  return kSuccess;
}

ErrorCode Debug::getTeamThreads(team_id team,
                                std::vector<thread_id> &threads) {
  DS2LOG(Debug, "enumerating PowerPC threads for team %d", team);

  threads.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamImages(team_id team, std::vector<area_id> &images) {
  DS2LOG(Debug, "enumerating PowerPC images for team %d", team);

  images.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamAreas(team_id team, std::vector<area_id> &areas) {
  DS2LOG(Debug, "enumerating PowerPC areas for team %d", team);

  areas.clear();
  return kSuccess;
}

ErrorCode Debug::enableSyscallTrace(team_id team, bool enable) {
  DS2LOG(Debug, "%s syscall tracing for PowerPC team %d",
         enable ? "enabling" : "disabling", team);

  return kSuccess;
}

bool Debug::_initialized = false;

} // namespace BeOS
} // namespace Host
} // namespace ds2
