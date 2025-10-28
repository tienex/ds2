//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// GNU/Hurd Host Platform Support
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/String.h"

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>

#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
  return "hurd";
}

char const *Platform::GetOSVendorName() {
  return "gnu";
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
    // GNU/Hurd version string
    char const *version = GetCachedUTSName()->version;
    if (version != nullptr) {
      strncpy(sBuild, version, sizeof(sBuild) - 1);
    }
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() {
  return "/boot/gnumach";  // GNU Mach kernel
}

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    // GNU/Hurd: /proc/self/exe should work (procfs translator)
    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
    if (len > 0) {
      path[len] = '\0';
    } else {
      // Fallback: Try /proc/<pid>/exe
      char proc_path[64];
      snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", getpid());
      len = readlink(proc_path, path, PATH_MAX);
      if (len > 0) {
        path[len] = '\0';
      }
    }
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  // GNU/Hurd: Read /proc/<pid>/stat if procfs translator is active
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);

  FILE *fp = fopen(path, "r");
  if (fp == nullptr) {
    return false;
  }

  int read_pid, ppid, pgrp, session, tty_nr, tpgid;
  unsigned int flags;
  unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
  long cutime, cstime, priority, nice, num_threads, itrealvalue;
  unsigned long long starttime;
  unsigned long vsize;
  long rss;
  char comm[256];
  char state;

  int ret = fscanf(fp, "%d %s %c %d %d %d %d %d %u "
                       "%lu %lu %lu %lu %lu %lu "
                       "%ld %ld %ld %ld %ld %ld "
                       "%llu %lu %ld",
                   &read_pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
                   &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
                   &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue,
                   &starttime, &vsize, &rss);

  fclose(fp);

  if (ret < 4) {
    return false;
  }

  info.pid = read_pid;
  info.parentPid = ppid;

  // Get UID/GID from /proc/<pid>/status
  snprintf(path, sizeof(path), "/proc/%d/status", pid);
  fp = fopen(path, "r");
  if (fp != nullptr) {
    char line[256];
    while (fgets(line, sizeof(line), fp) != nullptr) {
      if (strncmp(line, "Uid:", 4) == 0) {
        sscanf(line + 4, "%d", &info.realUid);
      } else if (strncmp(line, "Gid:", 4) == 0) {
        sscanf(line + 4, "%d", &info.realGid);
      }
    }
    fclose(fp);
  }

  // Remove parentheses from comm
  if (comm[0] == '(') {
    size_t len = strlen(comm);
    if (len > 0 && comm[len - 1] == ')') {
      comm[len - 1] = '\0';
      strncpy(info.name, comm + 1, sizeof(info.name) - 1);
    } else {
      strncpy(info.name, comm, sizeof(info.name) - 1);
    }
  } else {
    strncpy(info.name, comm, sizeof(info.name) - 1);
  }
  info.name[sizeof(info.name) - 1] = '\0';

  return true;
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {

  // GNU/Hurd: Enumerate /proc directory
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
  // GNU/Hurd: Try /proc/<pid>/task/<tid>/comm
  char path[128];
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
