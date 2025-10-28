//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/SystemV/ProcFS.h"

#if defined(OS_SYSV)

#include "DebugServer2/Host/Platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

using ds2::Host::Platform;

namespace ds2 {
namespace Host {
namespace SystemV {

ErrorCode ProcFS::wait(ProcessThreadId const &ptid, int *status) {
  int stat;
  pid_t ret;

  DS2LOG(Debug, "waiting for pid=%d", ptid.pid);

  ret = ::waitpid(ptid.pid, &stat, __WALL);
  if (ret < 0) {
    if (errno == ECHILD) {
      DS2LOG(Debug, "wait failed: process %d doesn't exist", ptid.pid);
      return kErrorProcessNotFound;
    } else {
      DS2LOG(Debug, "wait failed: %s", strerror(errno));
      return Platform::TranslateError();
    }
  }

  if (status != nullptr) {
    *status = stat;
  }

  DS2LOG(Debug, "waited for pid=%d, status=%#x", ptid.pid, stat);
  return kSuccess;
}

ErrorCode ProcFS::traceMe(bool disableASLR) {
  // On System V Unix, tracing is initiated by opening /proc/self/ctl
  // and writing PCSTOP to it
  int fd = open("/proc/self/ctl", O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  long cmd = PCSTOP;  // Request to stop for tracing
  ssize_t written = write(fd, &cmd, sizeof(cmd));
  close(fd);

  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode ProcFS::attach(ProcessId pid) {
  // Attaching is done by opening /proc/<pid>/ctl
  // The platform-specific traceThat will handle the actual attach operation
  return traceThat(pid);
}

ErrorCode ProcFS::detach(ProcessId pid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/ctl", pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  // PCRUN with PRCFAULT clears the current fault and resumes
  long cmd[2] = { PCRUN, PRCFAULT };
  ssize_t written = write(fd, cmd, sizeof(cmd));
  close(fd);

  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode ProcFS::suspend(ProcessThreadId const &ptid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/ctl", ptid.pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  long cmd = PCSTOP;
  ssize_t written = write(fd, &cmd, sizeof(cmd));
  close(fd);

  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode ProcFS::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                      int signal, Address const &address) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/ctl", ptid.pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  // PCRUN with PRSTEP for single-step
  long cmd[2] = { PCRUN, PRSTEP };
  ssize_t written = write(fd, cmd, sizeof(cmd));
  close(fd);

  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode ProcFS::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                        int signal, Address const &address) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/ctl", ptid.pid);

  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return Platform::TranslateError();
  }

  // PCRUN with PRCFAULT to resume after fault
  long cmd[2] = { PCRUN, PRCFAULT };
  if (signal != 0) {
    // If signal is specified, use PRCSIG to deliver it
    cmd[1] = PRCSIG | signal;
  }
  ssize_t written = write(fd, cmd, sizeof(cmd));
  close(fd);

  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode ProcFS::execute(ProcessThreadId const &ptid,
                         ProcessInfo const &pinfo, void const *code,
                         size_t length, uint64_t &result) {
  // Save current CPU state
  Architecture::CPUState savedState;
  ErrorCode error = readCPUState(ptid, pinfo, savedState);
  if (error != kSuccess)
    return error;

  // Allocate memory in target process for code
  Address codeAddress;
  // This would need platform-specific mmap syscall injection
  // For now, return not implemented
  return kErrorUnsupported;
}

int ProcFS::openProc(ProcessId pid, const char *file, int flags) {
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/%s", pid, file);
  return open(path, flags);
}

ErrorCode ProcFS::controlProc(int ctlfd, long cmd, void *data,
                             size_t dataSize) {
  ssize_t written;
  if (data != nullptr && dataSize > 0) {
    // Write command followed by data
    struct iovec iov[2];
    iov[0].iov_base = &cmd;
    iov[0].iov_len = sizeof(cmd);
    iov[1].iov_base = data;
    iov[1].iov_len = dataSize;
    written = writev(ctlfd, iov, 2);
    if (written != (ssize_t)(sizeof(cmd) + dataSize)) {
      return Platform::TranslateError();
    }
  } else {
    // Write command only
    written = write(ctlfd, &cmd, sizeof(cmd));
    if (written != sizeof(cmd)) {
      return Platform::TranslateError();
    }
  }
  return kSuccess;
}

ErrorCode ProcFS::waitProc(int ctlfd) {
  // Write PCWSTOP to wait for process to stop
  long cmd = PCWSTOP;
  ssize_t written = write(ctlfd, &cmd, sizeof(cmd));
  if (written != sizeof(cmd)) {
    return Platform::TranslateError();
  }
  return kSuccess;
}

} // namespace SystemV
} // namespace Host
} // namespace ds2

#endif // OS_SYSV
