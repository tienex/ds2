//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// OSF/1 Process - adapted from Hurd's Mach-based implementation
// OSF/1 uses ECOFF binary format, not ELF
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Target/OSF1/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#define super ds2::Target::POSIX::ECOFFProcess

namespace ds2 {
namespace Target {
namespace OSF1 {

ErrorCode Process::attach(int waitStatus) {
  if (waitStatus <= 0) {
    CHK(ptrace().attach(_pid));
    _flags |= kFlagAttachedProcess;
    CHK(ptrace().wait(_pid, &waitStatus));
    ptrace().traceThat(_pid);
  }

  // Create the main thread
  _currentThread = new Thread(this, _pid);
  _currentThread->updateStopInfo(waitStatus);

  return kSuccess;
}

ErrorCode Process::wait() {
  int status, signal;
  ProcessInfo info;
  ErrorCode err;
  pid_t tid;

  DS2ASSERT(!_threads.empty());

continue_waiting:
  err = ptrace().wait(_pid, &status);
  if (err != kSuccess)
    return err;

  DS2LOG(Debug, "stopped: status=%d", status);

  if (WIFEXITED(status)) {
    err = ptrace().wait(_pid, &status);
    DS2LOG(Debug, "exited: status=%d", status);
    _currentThread->updateStopInfo(status);
    _terminated = true;
    return kSuccess;
  }

  switch (_currentThread->_stopInfo.event) {
  case StopInfo::kEventNone:
    switch (_currentThread->_stopInfo.reason) {
    case StopInfo::kReasonNone:
      ptrace().resume(ProcessThreadId(_pid, tid), info);
      goto continue_waiting;
    default:
      DS2ASSERT(false);
      goto continue_waiting;
    }

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2LOG(Debug, "thread %d is exiting", tid);
    if (tid == _pid && _threads.size() == 1) {
      DS2LOG(Debug, "last thread is exiting");
      break;
    }
    removeThread(tid);
    goto continue_waiting;

  case StopInfo::kEventStop:
    if (getInfo(info) != kSuccess) {
      DS2LOG(Error, "couldn't get process info for pid %d", _pid);
      goto continue_waiting;
    }

    signal = _currentThread->_stopInfo.signal;

    if (signal == SIGSTOP || signal == SIGCHLD) {
      bool stepping = (_currentThread->state() == Thread::kStepped);

      if (signal == SIGSTOP) {
        signal = 0;
      } else {
        DS2LOG(Debug, "%s due to special signal, tid=%d status=%#x signal=%s",
               stepping ? "stepping" : "resuming", tid, status,
               strsignal(signal));
      }

      ErrorCode error;
      if (stepping) {
        error = ptrace().step(ProcessThreadId(_pid, tid), info, signal);
      } else {
        error = ptrace().resume(ProcessThreadId(_pid, tid), info, signal);
      }

      if (error != kSuccess) {
        DS2LOG(Warning, "cannot resume thread %d error=%d", tid, error);
      }

      goto continue_waiting;
    } else if (_passthruSignals.find(signal) != _passthruSignals.end()) {
      ptrace().resume(ProcessThreadId(_pid, tid), info, signal);
      goto continue_waiting;
    } else {
      // This is a signal that we want to transmit back to the debugger
      break;
    }
  }

  if (!(WIFEXITED(status) || WIFSIGNALED(status))) {
    // Suspend the process
    suspend();
  }

  if ((WIFEXITED(status) || WIFSIGNALED(status))) {
    _terminated = true;
  }

  return kSuccess;
}

ErrorCode Process::terminate() {
  ErrorCode error = super::terminate();
  if (error == kSuccess || error == kErrorProcessNotFound) {
    _terminated = !super::isAlive();
  }
  return error;
}

ErrorCode Process::suspend() {
  std::set<Thread *> threads;
  enumerateThreads([&](Thread *thread) { threads.insert(thread); });

  for (auto thread : threads) {
    Architecture::CPUState state;
    if (thread->state() != Thread::kRunning) {
      thread->readCPUState(state);
    }
    DS2LOG(Debug, "tid %d state %d at pc %#" PRIx64, thread->tid(),
           thread->state(),
           thread->state() == Thread::kStopped ? (uint64_t)state.pc() : 0);
    if (thread->state() == Thread::kRunning) {
      ErrorCode error;

      DS2LOG(Debug, "suspending tid %d", thread->tid());
      error = thread->suspend();

      if (error == kSuccess) {
        DS2LOG(Debug, "suspended tid %d at pc %#" PRIx64, thread->tid(),
               (uint64_t)state.pc());
        thread->readCPUState(state);
      } else if (error == kErrorProcessNotFound) {
        removeThread(thread->tid());
        DS2LOG(Debug, "tried to suspended tid %d which is already dead",
               thread->tid());
      } else {
        return error;
      }
    } else if (thread->state() == Thread::kTerminated) {
      removeThread(thread->tid());
    }
  }

  return kSuccess;
}

ds2::Host::POSIX::PTrace &Process::ptrace() const {
  return const_cast<Process *>(this)->_ptrace;
}

ds2::Host::OSF1::Mach &Process::mach() const {
  return const_cast<Process *>(this)->_mach;
}

ErrorCode Process::updateAuxiliaryVector() { return kSuccess; }

ErrorCode Process::updateInfo() {
  // OSF/1: Get process info
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  return kSuccess;
}

ErrorCode Process::getMemoryRegionInfo(Address const &address,
                                       MemoryRegionInfo &info) {
  if (!address.valid())
    return kErrorInvalidArgument;

  info.clear();

  return mach().getProcessMemoryRegion(_info.pid, address, info);
}

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  char buf[length];
  ErrorCode err;

  err = mach().readMemory(ProcessThreadId(_pid, _currentThread->tid()),
                          address, buf, length, count);
  if (err != kSuccess)
    return err;

  if (strnlen(buf, length) == length)
    return kErrorNameTooLong;

  str = std::string(buf);
  return kSuccess;
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  return mach().readMemory(ProcessThreadId(_pid, _currentThread->tid()),
                           address, data, length, count);
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  return mach().writeMemory(ProcessThreadId(_pid, _currentThread->tid()),
                            address, data, length, count);
}

ErrorCode Process::readCPUState(ThreadId tid, Architecture::CPUState &state,
                                uint32_t flags) {
  Thread *thread = _currentThread;

  if (thread == nullptr || thread->tid() != tid) {
    thread = findThread(tid);
    if (thread == nullptr)
      return kErrorProcessNotFound;
  }

  return mach().readCPUState(ProcessThreadId(_pid, tid), _info, state);
}

ErrorCode Process::writeCPUState(ThreadId tid,
                                 Architecture::CPUState const &state,
                                 uint32_t flags) {
  Thread *thread = _currentThread;

  if (thread == nullptr || thread->tid() != tid) {
    thread = findThread(tid);
    if (thread == nullptr)
      return kErrorProcessNotFound;
  }

  return mach().writeCPUState(ProcessThreadId(_pid, tid), _info, state);
}

ErrorCode Process::afterResume() { return kSuccess; }

} // namespace OSF1
} // namespace Target
} // namespace ds2
