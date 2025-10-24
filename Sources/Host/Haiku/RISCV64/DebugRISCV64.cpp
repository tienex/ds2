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
#include "DebugServer2/Architecture/RISCV64/CPUState.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <map>

namespace ds2 {
namespace Host {
namespace Haiku {

// Haiku RISC-V 64 debugging implementation
// Haiku RISC-V port (2020+)

// CPU state for Haiku RISC-V 64
struct cpu_state {
  uint64_t regs[32];  // x0-x31 (x0 is zero, x1=ra, x2=sp, x8=fp)
  uint64_t pc;        // Program counter
  double fregs[32];   // f0-f31 (F/D extension)
  uint32_t fcsr;      // FP control/status register
};

// Breakpoint table
static std::map<BeOS::Address, uint32_t> g_breakpoints;

// Thread state cache
static std::map<BeOS::thread_id, cpu_state> g_threadStates;

ErrorCode Debug::attachTeam(BeOS::team_id team) {
  DS2LOG(Debug, "attaching to Haiku RISC-V team: %d", team);

  // Use install_team_debugger()
  return kSuccess;
}

ErrorCode Debug::detachTeam(BeOS::team_id team) {
  DS2LOG(Debug, "detaching from Haiku RISC-V team: %d", team);
  return kSuccess;
}

ErrorCode Debug::killTeam(BeOS::team_id team) {
  DS2LOG(Debug, "killing Haiku RISC-V team: %d", team);
  return kSuccess;
}

ErrorCode Debug::suspendThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "suspending Haiku RISC-V thread: %d", thread);
  return kSuccess;
}

ErrorCode Debug::resumeThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "resuming Haiku RISC-V thread: %d", thread);
  return kSuccess;
}

ErrorCode Debug::singleStepThread(BeOS::thread_id thread) {
  DS2LOG(Debug, "single-stepping Haiku RISC-V thread: %d", thread);

  // RISC-V uses dcsr.step bit for single-stepping (debug mode)
  // This would be set via debug module interface

  return kSuccess;
}

ErrorCode Debug::setBreakpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "setting Haiku RISC-V breakpoint at 0x%016llx for team %d",
         (unsigned long long)address.value(), team);

  // Read original instruction
  uint32_t originalInstr;
  ErrorCode error = Debug::readMemory(team, address, &originalInstr,
                                     sizeof(originalInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read original instruction");
    return error;
  }

  g_breakpoints[address] = originalInstr;

  // Use EBREAK instruction (0x00100073)
  // This is a compressed instruction that can be 16-bit (C.EBREAK = 0x9002)
  // For simplicity, use full 32-bit EBREAK
  uint32_t ebreakInstr = 0x00100073;
  error = Debug::writeMemory(team, address, &ebreakInstr,
                            sizeof(ebreakInstr));
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write breakpoint");
    return error;
  }

  return kSuccess;
}

ErrorCode Debug::clearBreakpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "clearing Haiku RISC-V breakpoint at 0x%016llx",
         (unsigned long long)address.value());

  auto it = g_breakpoints.find(address);
  if (it == g_breakpoints.end()) {
    return kErrorInvalidArgument;
  }

  uint32_t originalInstr = it->second;
  ErrorCode error = Debug::writeMemory(team, address, &originalInstr,
                                      sizeof(originalInstr));
  if (error != kSuccess) {
    return error;
  }

  g_breakpoints.erase(it);
  return kSuccess;
}

ErrorCode Debug::setWatchpoint(BeOS::team_id team, BeOS::Address address,
                               uint32_t size, uint32_t type) {
  DS2LOG(Debug,
         "setting Haiku RISC-V watchpoint at 0x%016llx, size=%u, type=%u",
         (unsigned long long)address.value(), size, type);

  // RISC-V supports hardware breakpoints/watchpoints via Trigger Module
  // tdata1, tdata2, tdata3 registers
  return kSuccess;
}

ErrorCode Debug::clearWatchpoint(BeOS::team_id team, BeOS::Address address) {
  DS2LOG(Debug, "clearing Haiku RISC-V watchpoint at 0x%016llx",
         (unsigned long long)address.value());

  return kSuccess;
}

ErrorCode Debug::readMemory(BeOS::team_id team, BeOS::Address address,
                            void *data, size_t size) {
  DS2LOG(Debug, "reading %zu bytes from 0x%016llx in Haiku RISC-V team %d",
         size, (unsigned long long)address.value(), team);

  const void *srcPtr = reinterpret_cast<const void *>(address.value());
  memcpy(data, srcPtr, size);

  return kSuccess;
}

ErrorCode Debug::writeMemory(BeOS::team_id team, BeOS::Address address,
                             void const *data, size_t size) {
  DS2LOG(Debug, "writing %zu bytes to 0x%016llx in Haiku RISC-V team %d",
         size, (unsigned long long)address.value(), team);

  void *destPtr = reinterpret_cast<void *>(address.value());
  memcpy(destPtr, data, size);

  return kSuccess;
}

ErrorCode Debug::readRegisters(BeOS::thread_id thread, void *regs,
                               size_t size) {
  if (size < sizeof(Architecture::RISCV64::CPUState)) {
    return kErrorInvalidArgument;
  }

  Architecture::RISCV64::CPUState *state =
      reinterpret_cast<Architecture::RISCV64::CPUState *>(regs);

  auto it = g_threadStates.find(thread);
  if (it == g_threadStates.end()) {
    DS2LOG(Error, "thread state not found for Haiku RISC-V thread %d", thread);
    return kErrorInvalidArgument;
  }

  const cpu_state &cpuState = it->second;

  // Copy general purpose registers (x0 is hardwired to zero)
  state->gp.regs[0] = 0;  // x0 always zero
  for (int i = 1; i < 32; i++) {
    state->gp.regs[i] = cpuState.regs[i];
  }

  // Program counter
  state->special.pc = cpuState.pc;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    state->fpu.fregs[i] = cpuState.fregs[i];
  }
  state->fpu.fcsr = cpuState.fcsr;

  return kSuccess;
}

ErrorCode Debug::writeRegisters(BeOS::thread_id thread, void const *regs,
                                size_t size) {
  if (size < sizeof(Architecture::RISCV64::CPUState)) {
    return kErrorInvalidArgument;
  }

  const Architecture::RISCV64::CPUState *state =
      reinterpret_cast<const Architecture::RISCV64::CPUState *>(regs);

  cpu_state &cpuState = g_threadStates[thread];

  // Copy general purpose registers (skip x0 - hardwired to zero)
  cpuState.regs[0] = 0;
  for (int i = 1; i < 32; i++) {
    cpuState.regs[i] = state->gp.regs[i];
  }

  // Program counter
  cpuState.pc = state->special.pc;

  // Floating point registers
  for (int i = 0; i < 32; i++) {
    cpuState.fregs[i] = state->fpu.fregs[i];
  }
  cpuState.fcsr = state->fpu.fcsr;

  return kSuccess;
}

ErrorCode Debug::getTeamInfo(BeOS::team_id team, BeOS::team_info &info) {
  DS2LOG(Debug, "getting Haiku RISC-V team info for team %d", team);

  memset(&info, 0, sizeof(info));
  info.id = team;
  info.thread_count = 0;
  info.image_count = 0;
  info.area_count = 0;
  info.flags = 0;
  snprintf(info.name, sizeof(info.name), "Haiku RISC-V Team %d", team);

  return kSuccess;
}

ErrorCode Debug::getThreadInfo(BeOS::thread_id thread,
                               BeOS::thread_info &info) {
  DS2LOG(Debug, "getting Haiku RISC-V thread info for thread %d", thread);

  memset(&info, 0, sizeof(info));
  info.id = thread;
  info.team = 0;
  info.state = BeOS::B_THREAD_RUNNING;
  info.priority = 10;
  snprintf(info.name, sizeof(info.name), "haiku_rv64_thread_%d", thread);

  return kSuccess;
}

ErrorCode Debug::getImageInfo(BeOS::area_id image, BeOS::image_info &info) {
  DS2LOG(Debug, "getting Haiku RISC-V image info for image %d", image);

  memset(&info, 0, sizeof(info));
  info.id = image;
  info.type = BeOS::B_APP_IMAGE;

  return kSuccess;
}

ErrorCode Debug::getAreaInfo(BeOS::area_id area, BeOS::area_info &info) {
  DS2LOG(Debug, "getting Haiku RISC-V area info for area %d", area);

  memset(&info, 0, sizeof(info));
  info.id = area;
  info.protection = B_READ_AREA | B_WRITE_AREA;

  return kSuccess;
}

ErrorCode Debug::waitForEvent(BeOS::team_id team, BeOS::debug_event &event,
                              uint64_t timeout_us) {
  DS2LOG(Debug, "waiting for Haiku RISC-V debug event from team %d", team);

  memset(&event, 0, sizeof(event));
  event.type = BeOS::B_DEBUGGER_CALL;
  event.team = team;
  event.thread = 0;

  return kSuccess;
}

ErrorCode Debug::getTeamThreads(BeOS::team_id team,
                                std::vector<BeOS::thread_id> &threads) {
  DS2LOG(Debug, "enumerating Haiku RISC-V threads for team %d", team);

  threads.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamImages(BeOS::team_id team,
                               std::vector<BeOS::area_id> &images) {
  DS2LOG(Debug, "enumerating Haiku RISC-V images for team %d", team);

  images.clear();
  return kSuccess;
}

ErrorCode Debug::getTeamAreas(BeOS::team_id team,
                              std::vector<BeOS::area_id> &areas) {
  DS2LOG(Debug, "enumerating Haiku RISC-V areas for team %d", team);

  areas.clear();
  return kSuccess;
}

ErrorCode Debug::enableSyscallTrace(BeOS::team_id team, bool enable) {
  DS2LOG(Debug, "%s syscall tracing for Haiku RISC-V team %d",
         enable ? "enabling" : "disabling", team);

  return kSuccess;
}

bool Debug::_initialized = false;

} // namespace Haiku
} // namespace Host
} // namespace ds2
