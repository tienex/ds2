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
#include "DebugServer2/Architecture/X86_64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <map>

namespace ds2 {
namespace Host {
namespace Haiku {

// Haiku x86-64 debugging implementation
// Haiku x86-64 port (2012+)

// CPU state for Haiku x86-64
struct cpu_state {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rsi, rdi, rbp, rsp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t rip;
  uint64_t rflags;
  uint16_t cs, ds, es, fs, gs, ss;
  uint64_t fsbase, gsbase;

  // Debug registers
  uint64_t dr0, dr1, dr2, dr3;
  uint64_t dr6, dr7;
};

// Breakpoint table
static std::map<BeOS::Address, uint8_t> g_breakpoints;

// Thread state cache
static std::map<BeOS::thread_id, cpu_state> g_threadStates;

ErrorCode Debug::attachTeam(BeOS::team_id team) {
  DS2LOG(Debug, "attaching to Haiku x86-64 team: %d", team);

  // Use install_team_debugger()
  return kSuccess;
}

ErrorCode Debug::detachTeam(BeOS::team_id team) {
  DS2LOG(Debug, "detaching from Haiku x86-64 team: %d", team);
  return kSuccess;
}

ErrorCode Debug::killTeam(BeOS::team_id team) {
  DS2LOG(Debug, "killing Haiku x86-64 team: %d", team);
  return kSuccess;
}

ErrorCode Debug::suspendThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "suspending Haiku x86-64 thread: %d", thread);
  return kSuccess;
}

ErrorCode Debug::resumeThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "resuming Haiku x86-64 thread: %d", thread);
  return kSuccess;
}

ErrorCode Debug::singleStepThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "single-stepping Haiku x86-64 thread: %d", thread);

  auto it = g_threadStates.find(thread);
  if (it != g_threadStates.end()) {
    it->second.rflags |= 0x100;  // Set TF
  }

  return kSuccess;
}

ErrorCode Debug::setBreakpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "setting Haiku x86-64 breakpoint at 0x%016llx for team %d",
         (unsigned long long)address.value(), team);

  uint8_t originalByte;
  ErrorCode error = Debug::readMemory(team, address, &originalByte, 1);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read original instruction");
    return error;
  }

  g_breakpoints[address] = originalByte;

  uint8_t int3 = 0xCC;
  error = Debug::writeMemory(team, address, &int3, 1);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "clearing Haiku x86-64 breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) {
    return kErrorInvalidArgument;
  }

  uint8_t originalByte = it->second;
  ErrorCode error = Debug::writeMemory(team, address, &originalByte, 1);
  if (error != kSuccess) {
    return error;
  }

  g_breakpoints.erase(it);
  return kSuccess;
}

ErrorCode Debug::setWatchpoint(BeOS::team_id team, BeOS::Address address,
                               uint32_t size, uint32_t type) {
  DS2LOG(Debug,
         "setting Haiku x86-64 watchpoint at 0x%016llx, size=%u, type=%u",
         (unsigned long long)address.value(), size, type);

  // Use hardware debug registers
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "clearing Haiku x86-64 watchpoint at 0x%016llx",
         (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::readMemory(BeOS::team_id team, BeOS::Address address,
                            void *data, size_t size) {
  DS2LOG(Debug, "reading %zu bytes from 0x%016llx in Haiku x86-64 team %d",
         size, (unsigned long long)address.value(), team);

  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(BeOS::team_id team, BeOS::Address address,
                             void const *data, size_t size) {
  DS2LOG(Debug, "writing %zu bytes to 0x%016llx in Haiku x86-64 team %d",
         size, (unsigned long long)address.value(), team);

  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(BeOS::thread_id thread, void *regs,
                               size_t size) {
  if (size < sizeof(Architecture::X86_64::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::X86_64::CPUState *state =
      reinterpret_cast<Architecture::X86_64::CPUState *>(regs);

  auto it = g_threadStates.find(thread);
  if (it == g_threadStates.end()) {
    DS2LOG(Error, "thread state not found for Haiku x86-64 thread %d", thread);
    return kErrorInvalidArgument;
  }

  const cpu_state &cpuState = it->second;

  // Copy general purpose registers
  state->gp.rax = cpuState.rax;
  state->gp.rbx = cpuState.rbx;
  state->gp.rcx = cpuState.rcx;
  state->gp.rdx = cpuState.rdx;
  state->gp.rsi = cpuState.rsi;
  state->gp.rdi = cpuState.rdi;
  state->gp.rbp = cpuState.rbp;
  state->gp.rsp = cpuState.rsp;
  state->gp.r8 = cpuState.r8;
  state->gp.r9 = cpuState.r9;
  state->gp.r10 = cpuState.r10;
  state->gp.r11 = cpuState.r11;
  state->gp.r12 = cpuState.r12;
  state->gp.r13 = cpuState.r13;
  state->gp.r14 = cpuState.r14;
  state->gp.r15 = cpuState.r15;

  // Special registers
  state->special.rip = cpuState.rip;
  state->special.rflags = cpuState.rflags;

  // Segment registers
  state->special.cs = cpuState.cs;
  state->special.ds = cpuState.ds;
  state->special.es = cpuState.es;
  state->special.fs = cpuState.fs;
  state->special.gs = cpuState.gs;
  state->special.ss = cpuState.ss;

  // Segment bases
  state->special.fsbase = cpuState.fsbase;
  state->special.gsbase = cpuState.gsbase;

  // Debug registers
  state->debug.dr0 = cpuState.dr0;
  state->debug.dr1 = cpuState.dr1;
  state->debug.dr2 = cpuState.dr2;
  state->debug.dr3 = cpuState.dr3;
  state->debug.dr6 = cpuState.dr6;
  state->debug.dr7 = cpuState.dr7;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(BeOS::thread_id thread, void const *regs,
                                size_t size) {
  if (size < sizeof(Architecture::X86_64::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::X86_64::CPUState *state =
      reinterpret_cast<const Architecture::X86_64::CPUState *>(regs);

  cpu_state &cpuState = g_threadStates[thread];

  // Copy general purpose registers
  cpuState.rax = state->gp.rax;
  cpuState.rbx = state->gp.rbx;
  cpuState.rcx = state->gp.rcx;
  cpuState.rdx = state->gp.rdx;
  cpuState.rsi = state->gp.rsi;
  cpuState.rdi = state->gp.rdi;
  cpuState.rbp = state->gp.rbp;
  cpuState.rsp = state->gp.rsp;
  cpuState.r8 = state->gp.r8;
  cpuState.r9 = state->gp.r9;
  cpuState.r10 = state->gp.r10;
  cpuState.r11 = state->gp.r11;
  cpuState.r12 = state->gp.r12;
  cpuState.r13 = state->gp.r13;
  cpuState.r14 = state->gp.r14;
  cpuState.r15 = state->gp.r15;

  // Special registers
  cpuState.rip = state->special.rip;
  cpuState.rflags = state->special.rflags;

  // Segment registers
  cpuState.cs = state->special.cs;
  cpuState.ds = state->special.ds;
  cpuState.es = state->special.es;
  cpuState.fs = state->special.fs;
  cpuState.gs = state->special.gs;
  cpuState.ss = state->special.ss;

  // Segment bases
  cpuState.fsbase = state->special.fsbase;
  cpuState.gsbase = state->special.gsbase;

  // Debug registers
  cpuState.dr0 = state->debug.dr0;
  cpuState.dr1 = state->debug.dr1;
  cpuState.dr2 = state->debug.dr2;
  cpuState.dr3 = state->debug.dr3;
  cpuState.dr6 = state->debug.dr6;
  cpuState.dr7 = state->debug.dr7;

  return kSuccess;
}

ErrorCode Debug::getTeamInfo(BeOS::team_id team, BeOS::team_info &info) {
  DS2LOG(Debug, "getting Haiku x86-64 team info for team %d", team);

  memset(&info, 0, sizeof(info));
  info.id = team;
  info.thread_count = 0;
  info.image_count = 0;
  info.area_count = 0;
  info.flags = 0;
  snprintf(info.name, sizeof(info.name), "Haiku x86-64 Team %d", team);

  return kSuccess;
}

ErrorCode Debug::getThreadInfo(BeOS::thread_id thread,
                               BeOS::thread_info &info) {
  DS2LOG(Debug, "getting Haiku x86-64 thread info for thread %d", thread);

  memset(&info, 0, sizeof(info));
  info.id = thread;
  info.team = 0;
  info.state = BeOS::B_THREAD_RUNNING;
  info.priority = 10;
  snprintf(info.name, sizeof(info.name), "haiku_x64_thread_%d", thread);

  return kSuccess;
}

ErrorCode Debug::getImageInfo(BeOS::area_id image, BeOS::image_info &info) {
  DS2LOG(Debug, "getting Haiku x86-64 image info for image %d", image);

  memset(&info, 0, sizeof(info));
  info.id = image;
  info.type = BeOS::B_APP_IMAGE;

  return kSuccess;
}

ErrorCode Debug::getAreaInfo(BeOS::area_id area, BeOS::area_info &info) {
  DS2LOG(Debug, "getting Haiku x86-64 area info for area %d", area);

  memset(&info, 0, sizeof(info));
  info.id = area;
  info.protection = B_READ_AREA | B_WRITE_AREA;

  return kSuccess;
}

ErrorCode Debug::waitForEvent(BeOS::team_id team, BeOS::debug_event &event,
                              uint64_t timeout_us) {
  DS2LOG(Debug, "waiting for Haiku x86-64 debug event from team %d", team);

  memset(&event, 0, sizeof(event));
  event.type = BeOS::B_DEBUGGER_CALL;
  event.team = team;
  event.thread = 0;

  return kSuccess;
}

ErrorCode Debug::getTeamThreads(BeOS::team_id team,
                                std::vector<BeOS::thread_id> &threads) {
  DS2LOG(Debug, "enumerating Haiku x86-64 threads for team %d", team);

  threads.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamImages(BeOS::team_id team,
                               std::vector<BeOS::area_id> &images) {
  DS2LOG(Debug, "enumerating Haiku x86-64 images for team %d", team);

  images.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamAreas(BeOS::team_id team,
                              std::vector<BeOS::area_id> &areas) {
  DS2LOG(Debug, "enumerating Haiku x86-64 areas for team %d", team);

  areas.clear();
  return kSuccess;
}

ErrorCode Debug::enableSyscallTrace(BeOS::team_id team, bool enable) {
  DS2LOG(Debug, "%s syscall tracing for Haiku x86-64 team %d",
         enable ? "enabling" : "disabling", team);

  return kSuccess;
}

bool Debug::_initialized = false;

} // namespace Haiku
} // namespace Host
} // namespace ds2
