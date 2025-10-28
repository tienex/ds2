//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Solaris / illumos Host Platform Support
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/String.h"

#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <procfs.h>
#include <dirent.h>

#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
#if defined(__illumos__)
  return "illumos";
#else
  return "solaris";
#endif
}

char const *Platform::GetOSVendorName() {
  static char vendor[256] = {'\0'};

  if (vendor[0] == '\0') {
#if defined(__illumos__)
    // illumos distributions: OpenIndiana, OmniOS, SmartOS, etc.
    if (sysinfo(SI_PLATFORM, vendor, sizeof(vendor)) < 0) {
      strcpy(vendor, "illumos");
    }
#else
    // Oracle Solaris
    strcpy(vendor, "oracle");
#endif
  }

  return vendor;
}

static struct utsname const *GetCachedUTSName() {
  static struct utsname sUName = {"", "", "", "", "", ""};
  if (sUName.release[0] == '\0') {
    ::uname(&sUName);
  }
  return &sUName;
}

char const *Platform::GetOSVersion() {
  static char version[64] = {'\0'};

  if (version[0] == '\0') {
    // Solaris: Get version from sysinfo
    if (sysinfo(SI_RELEASE, version, sizeof(version)) < 0) {
      // Fallback to uname
      strncpy(version, GetCachedUTSName()->release, sizeof(version) - 1);
    }
  }

  return version;
}

char const *Platform::GetOSBuild() {
  static char sBuild[64] = {'\0'};

  if (sBuild[0] == '\0') {
    // Solaris version string
    char const *version = GetCachedUTSName()->version;
    if (version != nullptr) {
      strncpy(sBuild, version, sizeof(sBuild) - 1);
    }
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() {
#if defined(__sparc) || defined(__sparc__)
  return "/platform/sun4u/kernel/sparcv9/unix";  // SPARC kernel
#elif defined(__i386) || defined(__x86_64)
  return "/platform/i86pc/kernel/amd64/unix";    // x86-64 kernel
#else
  return "/kernel/genunix";
#endif
}

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    // Solaris/illumos: /proc/self/path/a.out
    ssize_t len = readlink("/proc/self/path/a.out", path, PATH_MAX);
    if (len > 0) {
      path[len] = '\0';
    } else {
      // Alternative: getexecname()
      const char *exec = getexecname();
      if (exec != nullptr) {
        if (exec[0] == '/') {
          strncpy(path, exec, PATH_MAX);
        } else {
          // Relative path, make it absolute
          char cwd[PATH_MAX];
          if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            snprintf(path, PATH_MAX, "%s/%s", cwd, exec);
          }
        }
      }
    }
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  // Solaris: Read /proc/<pid>/psinfo
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/psinfo", pid);

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return false;
  }

  psinfo_t psinfo;
  if (read(fd, &psinfo, sizeof(psinfo)) == sizeof(psinfo)) {
    info.pid = psinfo.pr_pid;
    info.parentPid = psinfo.pr_ppid;
    info.realUid = psinfo.pr_uid;
    info.realGid = psinfo.pr_gid;

    // Copy command name
    strncpy(info.name, psinfo.pr_fname, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';

    close(fd);
    return true;
  }

  close(fd);
  return false;
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {

  // Solaris: Enumerate /proc directory
  DIR *dir = opendir("/proc");
  if (dir == nullptr) {
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    // Skip non-numeric entries
    if (!isdigit(entry->d_name[0])) {
      continue;
    }

    ProcessId pid = atoi(entry->d_name);
    ProcessInfo info;

    if (GetProcessInfo(pid, info)) {
      if (!allUsers && info.realUid != uid) {
        continue;
      }

      cb(info);
    }
  }

  closedir(dir);
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  // Solaris: Read /proc/<pid>/lwp/<tid>/lwpname (Solaris 11.3+)
  char path[128];
  snprintf(path, sizeof(path), "/proc/%d/lwp/%lld/lwpname", pid, (long long)tid);

  int fd = open(path, O_RDONLY);
  if (fd >= 0) {
    char name[256];
    ssize_t len = read(fd, name, sizeof(name) - 1);
    close(fd);

    if (len > 0) {
      name[len] = '\0';
      // Remove trailing newline
      if (name[len - 1] == '\n') {
        name[len - 1] = '\0';
      }
      return std::string(name);
    }
  }

  return std::string();
}

} // namespace Host
} // namespace ds2
