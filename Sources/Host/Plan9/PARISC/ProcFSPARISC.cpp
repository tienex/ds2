//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/PARISC/CPUState.h"
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

using namespace Architecture::PARISC;

// Plan 9 PA-RISC register file format (text-based)
// Format: "R0=xxxxxxxx R1=xxxxxxxx ... R31=xxxxxxxx\n"
//         "SR0=xxxxxxxx SR1=xxxxxxxx ... SR7=xxxxxxxx\n"
//         "IAOQ_HEAD=xxxxxxxx IAOQ_TAIL=xxxxxxxx\n"
//         "SAR=xxxxxxxx PSW=xxxxxxxx\n"

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
  char buffer[4096];
  ssize_t nread = ::read(fd, buffer, sizeof(buffer) - 1);
  ::close(fd);

  if (nread < 0)
    return Platform::TranslateError();

  buffer[nread] = '\0';

  // Parse text-based register format
  // Line 1: R0-R7
  const char *line = buffer;
  for (int i = 0; i < 8; i++) {
    char regname[16];
    uint32_t value;
    snprintf(regname, sizeof(regname), "R%d=", i);
    const char *pos = strstr(line, regname);
    if (!pos)
      return kErrorInvalidArgument;
    if (sscanf(pos + strlen(regname), "%x", &value) != 1)
      return kErrorInvalidArgument;
    state.gp.regs[i] = value;
  }

  // Line 2: R8-R15
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  for (int i = 8; i < 16; i++) {
    char regname[16];
    uint32_t value;
    snprintf(regname, sizeof(regname), "R%d=", i);
    const char *pos = strstr(line, regname);
    if (!pos)
      return kErrorInvalidArgument;
    if (sscanf(pos + strlen(regname), "%x", &value) != 1)
      return kErrorInvalidArgument;
    state.gp.regs[i] = value;
  }

  // Line 3: R16-R23
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  for (int i = 16; i < 24; i++) {
    char regname[16];
    uint32_t value;
    snprintf(regname, sizeof(regname), "R%d=", i);
    const char *pos = strstr(line, regname);
    if (!pos)
      return kErrorInvalidArgument;
    if (sscanf(pos + strlen(regname), "%x", &value) != 1)
      return kErrorInvalidArgument;
    state.gp.regs[i] = value;
  }

  // Line 4: R24-R31
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  for (int i = 24; i < 32; i++) {
    char regname[16];
    uint32_t value;
    snprintf(regname, sizeof(regname), "R%d=", i);
    const char *pos = strstr(line, regname);
    if (!pos)
      return kErrorInvalidArgument;
    if (sscanf(pos + strlen(regname), "%x", &value) != 1)
      return kErrorInvalidArgument;
    state.gp.regs[i] = value;
  }

  // Line 5: Space registers
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  for (int i = 0; i < 8; i++) {
    char regname[16];
    uint32_t value;
    snprintf(regname, sizeof(regname), "SR%d=", i);
    const char *pos = strstr(line, regname);
    if (!pos)
      return kErrorInvalidArgument;
    if (sscanf(pos + strlen(regname), "%x", &value) != 1)
      return kErrorInvalidArgument;
    state.sr.regs[i] = value;
  }

  // Line 6: Special registers
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  if (sscanf(line, "IAOQ_HEAD=%x IAOQ_TAIL=%x", &state.special.iaoq_head,
             &state.special.iaoq_tail) != 2) {
    return kErrorInvalidArgument;
  }

  // Line 7: SAR and PSW
  line = strchr(line, '\n');
  if (!line)
    return kErrorInvalidArgument;
  line++;

  if (sscanf(line, "SAR=%x PSW=%x", &state.special.sar, &state.special.psw) !=
      2) {
    return kErrorInvalidArgument;
  }

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
  char buffer[4096];
  int len = 0;

  // Line 1: R0-R7
  for (int i = 0; i < 8; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "R%d=%08X ",
                   i, state.gp.regs[i]);
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

  // Line 2: R8-R15
  for (int i = 8; i < 16; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "R%d=%08X ",
                   i, state.gp.regs[i]);
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

  // Line 3: R16-R23
  for (int i = 16; i < 24; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "R%d=%08X ",
                   i, state.gp.regs[i]);
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

  // Line 4: R24-R31
  for (int i = 24; i < 32; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "R%d=%08X ",
                   i, state.gp.regs[i]);
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

  // Line 5: Space registers
  for (int i = 0; i < 8; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "SR%d=%08X ",
                   i, state.sr.regs[i]);
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "\n");

  // Line 6: Special registers
  len += snprintf(buffer + len, sizeof(buffer) - len,
                 "IAOQ_HEAD=%08X IAOQ_TAIL=%08X\n", state.special.iaoq_head,
                 state.special.iaoq_tail);

  // Line 7: SAR and PSW
  len += snprintf(buffer + len, sizeof(buffer) - len, "SAR=%08X PSW=%08X\n",
                 state.special.sar, state.special.psw);

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

} // namespace Plan9
} // namespace Host
} // namespace ds2
