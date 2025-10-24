//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86/CPUState.h"
#include "DebugServer2/Host/Plan9/ProcFS.h"
#include "DebugServer2/Host/Platform.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace Plan9 {

using namespace Architecture::X86;

// Plan 9 register file format (text-based)
// Format: "AX=xxxxxxxx BX=xxxxxxxx CX=xxxxxxxx DX=xxxxxxxx\n"
//         "SI=xxxxxxxx DI=xxxxxxxx BP=xxxxxxxx\n"
//         "CS=xxxx DS=xxxx ES=xxxx FS=xxxx GS=xxxx SS=xxxx\n"
//         "PC=xxxxxxxx SP=xxxxxxxx FLAGS=xxxxxxxx\n"

ErrorCode ProcFS::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  // Open /proc/<pid>/regs
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/regs", ptid.pid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Read register file (text format)
  char buffer[1024];
  ssize_t nread = ::read(fd, buffer, sizeof(buffer) - 1);
  ::close(fd);

  if (nread < 0)
    return Platform::TranslateError();

  buffer[nread] = '\0';

  // Parse text-based register format
  uint32_t ax, bx, cx, dx, si, di, bp, sp, pc, flags;
  uint16_t cs, ds, es, fs, gs, ss;

  // Parse general purpose registers
  if (sscanf(buffer, "AX=%x BX=%x CX=%x DX=%x", &ax, &bx, &cx, &dx) != 4) {
    return kErrorInvalidArgument;
  }

  const char *line2 = strchr(buffer, '\n');
  if (!line2)
    return kErrorInvalidArgument;
  line2++;

  if (sscanf(line2, "SI=%x DI=%x BP=%x", &si, &di, &bp) != 3) {
    return kErrorInvalidArgument;
  }

  const char *line3 = strchr(line2, '\n');
  if (!line3)
    return kErrorInvalidArgument;
  line3++;

  if (sscanf(line3, "CS=%hx DS=%hx ES=%hx FS=%hx GS=%hx SS=%hx", &cs, &ds, &es,
             &fs, &gs, &ss) != 6) {
    return kErrorInvalidArgument;
  }

  const char *line4 = strchr(line3, '\n');
  if (!line4)
    return kErrorInvalidArgument;
  line4++;

  if (sscanf(line4, "PC=%x SP=%x FLAGS=%x", &pc, &sp, &flags) != 3) {
    return kErrorInvalidArgument;
  }

  // Fill in CPUState
  state.gp.eax = ax;
  state.gp.ebx = bx;
  state.gp.ecx = cx;
  state.gp.edx = dx;
  state.gp.esi = si;
  state.gp.edi = di;
  state.gp.ebp = bp;
  state.gp.esp = sp;
  state.gp.eip = pc;
  state.gp.eflags = flags;

  state.gp.cs = cs;
  state.gp.ds = ds;
  state.gp.es = es;
  state.gp.fs = fs;
  state.gp.gs = gs;
  state.gp.ss = ss;

  return kSuccess;
}

ErrorCode ProcFS::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  // Open /proc/<pid>/regs for writing
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/regs", ptid.pid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Format register state as text
  char buffer[1024];
  int len = snprintf(buffer, sizeof(buffer),
                    "AX=%08X BX=%08X CX=%08X DX=%08X\n"
                    "SI=%08X DI=%08X BP=%08X\n"
                    "CS=%04X DS=%04X ES=%04X FS=%04X GS=%04X SS=%04X\n"
                    "PC=%08X SP=%08X FLAGS=%08X\n",
                    state.gp.eax, state.gp.ebx, state.gp.ecx, state.gp.edx,
                    state.gp.esi, state.gp.edi, state.gp.ebp, state.gp.cs,
                    state.gp.ds, state.gp.es, state.gp.fs, state.gp.gs,
                    state.gp.ss, state.gp.eip, state.gp.esp, state.gp.eflags);

  if (len < 0 || len >= (int)sizeof(buffer)) {
    ::close(fd);
    return kErrorInvalidArgument;
  }

  ssize_t nwritten = ::write(fd, buffer, len);
  ::close(fd);

  if (nwritten != len)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode ProcFS::sendControlCommand(ProcessId pid, const char *command) {
  // Open /proc/<pid>/ctl
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/ctl", pid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0)
    return Platform::TranslateError();

  ssize_t len = strlen(command);
  ssize_t nwritten = ::write(fd, command, len);
  ::close(fd);

  if (nwritten != len)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode ProcFS::attach(ProcessId pid) {
  // Plan 9: send "hang" command to suspend process
  return sendControlCommand(pid, "hang");
}

ErrorCode ProcFS::detach(ProcessId pid) {
  // Plan 9: send "start" command to resume process
  return sendControlCommand(pid, "start");
}

ErrorCode ProcFS::suspend(ProcessThreadId const &ptid) {
  return sendControlCommand(ptid.pid, "hang");
}

ErrorCode ProcFS::resume(ProcessThreadId const &ptid) {
  return sendControlCommand(ptid.pid, "start");
}

ErrorCode ProcFS::step(ProcessThreadId const &ptid) {
  // Plan 9: send "step" command for single-step
  return sendControlCommand(ptid.pid, "step");
}

ErrorCode ProcFS::kill(ProcessId pid) {
  return sendControlCommand(pid, "kill");
}

ErrorCode ProcFS::readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *nread) {
  // Open /proc/<pid>/mem
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/mem", ptid.pid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Seek to address
  if (::lseek(fd, address.value(), SEEK_SET) < 0) {
    ::close(fd);
    return Platform::TranslateError();
  }

  // Read memory
  ssize_t result = ::read(fd, buffer, length);
  ::close(fd);

  if (result < 0)
    return Platform::TranslateError();

  if (nread)
    *nread = result;

  return kSuccess;
}

ErrorCode ProcFS::writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *nwritten) {
  // Open /proc/<pid>/mem
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/mem", ptid.pid);

  int fd = ::open(path, O_WRONLY);
  if (fd < 0)
    return Platform::TranslateError();

  // Seek to address
  if (::lseek(fd, address.value(), SEEK_SET) < 0) {
    ::close(fd);
    return Platform::TranslateError();
  }

  // Write memory
  ssize_t result = ::write(fd, buffer, length);
  ::close(fd);

  if (result < 0)
    return Platform::TranslateError();

  if (nwritten)
    *nwritten = result;

  return kSuccess;
}

ErrorCode ProcFS::readText(ProcessId pid, Address const &address, void *buffer,
                           size_t length, size_t *nread) {
  // Open /proc/<pid>/text for reading code segment
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/text", pid);

  int fd = ::open(path, O_RDONLY);
  if (fd < 0)
    return Platform::TranslateError();

  if (::lseek(fd, address.value(), SEEK_SET) < 0) {
    ::close(fd);
    return Platform::TranslateError();
  }

  ssize_t result = ::read(fd, buffer, length);
  ::close(fd);

  if (result < 0)
    return Platform::TranslateError();

  if (nread)
    *nread = result;

  return kSuccess;
}

bool ProcFS::isInferno() {
  // Check if running under Inferno (Dis virtual machine)
  // Inferno has /dev/sysctl
  int fd = ::open("/dev/sysctl", O_RDONLY);
  if (fd >= 0) {
    ::close(fd);
    return true;
  }
  return false;
}

} // namespace Plan9
} // namespace Host
} // namespace ds2
