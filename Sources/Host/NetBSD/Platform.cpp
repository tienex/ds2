//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// NetBSD Host Platform Support
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/String.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
  return "netbsd";
}

char const *Platform::GetOSVendorName() {
  return "netbsd";
}

static struct utsname const *GetCachedUTSName() {
  static struct utsname sUName = {"", "", "", "", "", ""};
  if (sUName.release[0] == '\0') {
    ::uname(&sUName);
  }
  return &sUName;
}

char const *Platform::GetOSVersion() {
  return GetCachedUTSName()->release;
}

char const *Platform::GetOSBuild() {
  static char sBuild[64] = {'\0'};

  if (sBuild[0] == '\0') {
    // NetBSD version string format: "NetBSD X.Y.Z (BUILD_INFO)"
    char const *version = GetCachedUTSName()->version;
    if (version != nullptr) {
      strncpy(sBuild, version, sizeof(sBuild) - 1);
    }
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() {
  return "/netbsd";  // NetBSD kernel path
}

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    // NetBSD: /proc/curproc/exe (if procfs mounted)
    ssize_t len = readlink("/proc/curproc/exe", path, PATH_MAX);
    if (len > 0) {
      path[len] = '\0';
    } else {
      // Fallback: use sysctl
      int mib[4] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_PATHNAME };
      size_t size = sizeof(path);
      if (sysctl(mib, 4, path, &size, NULL, 0) == 0) {
        path[size] = '\0';
      }
    }
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  // NetBSD: Use sysctl KERN_PROC2 or /proc if available
  int mib[6] = { CTL_KERN, KERN_PROC2, KERN_PROC_PID, pid, sizeof(struct kinfo_proc2), 1 };
  struct kinfo_proc2 kp;
  size_t size = sizeof(kp);

  if (sysctl(mib, 6, &kp, &size, NULL, 0) == 0) {
    info.pid = kp.p_pid;
    info.parentPid = kp.p_ppid;
    info.realUid = kp.p_ruid;
    info.realGid = kp.p_rgid;

    // Copy command name
    strncpy(info.name, kp.p_comm, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';

    return true;
  }

  return false;
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {

  // NetBSD: Use sysctl KERN_PROC2 to enumerate all processes
  int mib[6] = { CTL_KERN, KERN_PROC2, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), 0 };
  size_t size;

  // Get size
  if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
    return;
  }

  // Get count
  mib[5] = size / sizeof(struct kinfo_proc2);
  struct kinfo_proc2 *procs = (struct kinfo_proc2 *)malloc(size);
  if (procs == NULL) {
    return;
  }

  if (sysctl(mib, 6, procs, &size, NULL, 0) < 0) {
    free(procs);
    return;
  }

  size_t count = size / sizeof(struct kinfo_proc2);
  for (size_t i = 0; i < count; i++) {
    if (!allUsers && procs[i].p_ruid != uid) {
      continue;
    }

    ProcessInfo info;
    info.pid = procs[i].p_pid;
    info.parentPid = procs[i].p_ppid;
    info.realUid = procs[i].p_ruid;
    info.realGid = procs[i].p_rgid;
    strncpy(info.name, procs[i].p_comm, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';

    cb(info);
  }

  free(procs);
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  // NetBSD doesn't have easy thread name API in older versions
  // Return empty string or use /proc if available
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/task/%lld/comm", pid, (long long)tid);

  FILE *fp = fopen(path, "r");
  if (fp != nullptr) {
    char name[256];
    if (fgets(name, sizeof(name), fp) != nullptr) {
      // Remove trailing newline
      size_t len = strlen(name);
      if (len > 0 && name[len - 1] == '\n') {
        name[len - 1] = '\0';
      }
      fclose(fp);
      return std::string(name);
    }
    fclose(fp);
  }

  return std::string();
}

} // namespace Host
} // namespace ds2
