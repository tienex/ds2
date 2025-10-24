//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// GNU/Hurd Mach implementation - adapted from Darwin's Mach for GNU Mach
//

#include "DebugServer2/Host/Hurd/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>
#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Hurd {

task_t Mach::getMachTask(ProcessId pid) {
  task_t self = mach_task_self();
  task_t task;

  kern_return_t kret = task_for_pid(self, pid, &task);
  if (kret != KERN_SUCCESS) {
    return TASK_NULL;
  }

  return task;
}

thread_t Mach::getMachThread(ProcessThreadId const &ptid) {
  thread_t *thread_list;
  mach_msg_type_number_t thread_count;

  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return THREAD_NULL;

  kern_return_t kret = task_threads(task, &thread_list, &thread_count);
  if (kret != KERN_SUCCESS) {
    return THREAD_NULL;
  }

  // Find the specific thread by TID
  // GNU Mach: For now, use first thread (needs improvement for multi-thread)
  thread_t thread = thread_list[0];

  vm_deallocate(mach_task_self(), (vm_address_t)thread_list,
                thread_count * sizeof(thread_t));

  return thread;
}

ErrorCode Mach::readMemory(ProcessThreadId const &ptid, Address const &address,
                           void *buffer, size_t length, size_t *count) {
  vm_size_t curr_bytes_read = 0;

  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return kErrorProcessNotFound;

  // GNU Mach uses vm_read instead of mach_vm_read_overwrite
  vm_offset_t data;
  mach_msg_type_number_t data_count;

  kern_return_t kret = vm_read(task, (vm_address_t)address.value(), length,
                                &data, &data_count);
  if (kret != KERN_SUCCESS)
    return kErrorInvalidAddress;

  // Copy data to buffer
  size_t copy_size = (data_count < length) ? data_count : length;
  memcpy(buffer, (void *)data, copy_size);
  curr_bytes_read = copy_size;

  // Deallocate the data returned by vm_read
  vm_deallocate(mach_task_self(), data, data_count);

  if (count != nullptr) {
    *count = curr_bytes_read;
  }

  return kSuccess;
}

ErrorCode Mach::writeMemory(ProcessThreadId const &ptid, Address const &address,
                            void const *buffer, size_t length, size_t *count) {
  ErrorCode error = kSuccess;

  task_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  // Get current protection to restore later
  vm_address_t region_start = (vm_address_t)address.value();
  vm_size_t region_size;
  vm_region_basic_info_data_t region_info;
  mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT;
  mach_port_t object_name;

  kern_return_t kret =
      vm_region(task, &region_start, &region_size, VM_REGION_BASIC_INFO,
                (vm_region_info_t)&region_info, &info_count, &object_name);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  // GNU Mach: Set write protection temporarily
  kret = vm_protect(task, (vm_address_t)address.value(), length, FALSE,
                    VM_PROT_WRITE | VM_PROT_READ);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  // Write memory
  kret = vm_write(task, (vm_address_t)address.value(), (vm_offset_t)buffer,
                  length);
  if (kret != KERN_SUCCESS) {
    error = kErrorUnknown;
  } else if (count != nullptr) {
    *count = length;
  }

  // Restore original protection
  kret = vm_protect(task, (vm_address_t)address.value(), length, FALSE,
                    region_info.protection);
  if (kret != KERN_SUCCESS && error == kSuccess) {
    error = kErrorUnknown;
  }

  return error;
}

ErrorCode Mach::suspend(ProcessThreadId const &ptid) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  kern_return_t kret = thread_suspend(thread);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  return kSuccess;
}

ErrorCode Mach::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                     int signal, Address const &address) {
  // GNU Mach doesn't have native single-step support
  // This would need to be implemented using breakpoints
  return kErrorUnsupported;
}

ErrorCode Mach::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  kern_return_t kret = thread_resume(thread);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  return kSuccess;
}

ErrorCode Mach::getProcessMemoryRegion(ProcessId pid, Address const &address,
                                       MemoryRegionInfo &region) {
  task_t task = getMachTask(pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  vm_region_basic_info_data_t region_info;
  mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT;
  vm_address_t start = (vm_address_t)address.value();
  vm_size_t size;
  mach_port_t object_name;

  kern_return_t kret = vm_region(task, &start, &size, VM_REGION_BASIC_INFO,
                                 (vm_region_info_t)&region_info, &count,
                                 &object_name);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "unable to retrieve region (0x%llx) info", (uint64_t)start);
    return kErrorUnknown;
  }

  region.start = start;
  region.length = size;

  if ((region_info.protection & VM_PROT_READ) == VM_PROT_READ)
    region.protection |= ds2::kProtectionRead;
  if ((region_info.protection & VM_PROT_WRITE) == VM_PROT_WRITE)
    region.protection |= ds2::kProtectionWrite;
  if ((region_info.protection & VM_PROT_EXECUTE) == VM_PROT_EXECUTE)
    region.protection |= ds2::kProtectionExecute;

  return kSuccess;
}

ErrorCode Mach::getThreadInfo(ProcessThreadId const &ptid, void *info) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  // GNU Mach thread info
  unsigned int thread_info_count = THREAD_BASIC_INFO_COUNT;
  kern_return_t kret = thread_info(thread, THREAD_BASIC_INFO,
                                   (thread_info_t)info, &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

ErrorCode Mach::readCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &info,
                              Architecture::CPUState &state) {
  // This will be implemented architecture-specific
  // in Host/Hurd/X86/MachX86.cpp and X86_64/MachX86_64.cpp
  return kErrorUnsupported;
}

ErrorCode Mach::writeCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &info,
                               Architecture::CPUState const &state) {
  // This will be implemented architecture-specific
  // in Host/Hurd/X86/MachX86.cpp and X86_64/MachX86_64.cpp
  return kErrorUnsupported;
}

} // namespace Hurd
} // namespace Host
} // namespace ds2
