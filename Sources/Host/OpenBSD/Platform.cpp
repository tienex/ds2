//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OpenBSD Host Platform Support
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
#include <kvm.h>

#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
  return "openbsd";
}

char const *Platform::GetOSVendorName() {
  return "openbsd";
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
    // OpenBSD version string
    char const *version = GetCachedUTSName()->version;
    if (version != nullptr) {
      strncpy(sBuild, version, sizeof(sBuild) - 1);
    }
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() {
  return "/bsd";  // OpenBSD kernel path
}

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    // OpenBSD: Use sysctl CTL_KERN.KERN_PROC_ARGS.pid.KERN_PROC_ARGV
    int mib[4] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV };
    size_t size;

    if (sysctl(mib, 4, NULL, &size, NULL, 0) == 0) {
      char **argv = (char **)malloc(size);
      if (argv != NULL) {
        if (sysctl(mib, 4, argv, &size, NULL, 0) == 0 && argv[0] != NULL) {
          // Resolve to absolute path
          if (realpath(argv[0], path) == NULL) {
            strncpy(path, argv[0], PATH_MAX);
          }
        }
        free(argv);
      }
    }
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  // OpenBSD: Use kvm_getprocs or sysctl
  int mib[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), 1 };
  struct kinfo_proc kp;
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

  // OpenBSD: Use sysctl KERN_PROC
  int mib[6] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), 0 };
  size_t size;

  // Get size
  if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
    return;
  }

  // Allocate buffer
  struct kinfo_proc *procs = (struct kinfo_proc *)malloc(size);
  if (procs == NULL) {
    return;
  }

  // Get process list
  mib[5] = size / sizeof(struct kinfo_proc);
  if (sysctl(mib, 6, procs, &size, NULL, 0) < 0) {
    free(procs);
    return;
  }

  size_t count = size / sizeof(struct kinfo_proc);
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
  // OpenBSD doesn't have user-visible thread names in most versions
  // Return empty string
  return std::string();
}

} // namespace Host
} // namespace ds2
