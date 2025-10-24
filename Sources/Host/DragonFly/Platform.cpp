//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// DragonFly BSD Host Platform Support
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/String.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/user.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
  return "dragonfly";
}

char const *Platform::GetOSVendorName() {
  return "dragonfly";
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
    // DragonFly BSD version string
    char const *version = GetCachedUTSName()->version;
    if (version != nullptr) {
      strncpy(sBuild, version, sizeof(sBuild) - 1);
    }
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() {
  return "/kernel";  // DragonFly BSD kernel path
}

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    // DragonFly BSD: Use sysctl KERN_PROC.KERN_PROC_PATHNAME
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t size = sizeof(path);

    if (sysctl(mib, 4, path, &size, NULL, 0) == 0) {
      path[size] = '\0';
    } else {
      // Fallback: read /proc/curproc/file if procfs mounted
      ssize_t len = readlink("/proc/curproc/file", path, PATH_MAX);
      if (len > 0) {
        path[len] = '\0';
      }
    }
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  // DragonFly BSD: Use sysctl KERN_PROC
  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
  struct kinfo_proc kp;
  size_t size = sizeof(kp);

  if (sysctl(mib, 4, &kp, &size, NULL, 0) == 0) {
    info.pid = kp.kp_pid;
    info.parentPid = kp.kp_ppid;
    info.realUid = kp.kp_ruid;
    info.realGid = kp.kp_rgid;

    // Copy command name
    strncpy(info.name, kp.kp_comm, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';

    return true;
  }

  return false;
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {

  // DragonFly BSD: Use sysctl KERN_PROC.KERN_PROC_ALL
  int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
  size_t size;

  // Get size
  if (sysctl(mib, 3, NULL, &size, NULL, 0) < 0) {
    return;
  }

  // Allocate buffer
  struct kinfo_proc *procs = (struct kinfo_proc *)malloc(size);
  if (procs == NULL) {
    return;
  }

  // Get process list
  if (sysctl(mib, 3, procs, &size, NULL, 0) < 0) {
    free(procs);
    return;
  }

  size_t count = size / sizeof(struct kinfo_proc);
  for (size_t i = 0; i < count; i++) {
    if (!allUsers && procs[i].kp_ruid != uid) {
      continue;
    }

    ProcessInfo info;
    info.pid = procs[i].kp_pid;
    info.parentPid = procs[i].kp_ppid;
    info.realUid = procs[i].kp_ruid;
    info.realGid = procs[i].kp_rgid;
    strncpy(info.name, procs[i].kp_comm, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';

    cb(info);
  }

  free(procs);
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  // DragonFly BSD: Try /proc if available
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
