//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Unified Mach implementation for Darwin (XNU Mach) and GNU/Hurd (GNU Mach)
//

#include "DebugServer2/Host/Mach/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <limits>

// Platform-specific Mach headers
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#elif defined(__GNU__)
#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#endif

#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Mach {

task_t MachInterface::getMachTask(ProcessId pid) {
  task_t self = mach_task_self();
  task_t task;

  kern_return_t kret = task_for_pid(self, pid, &task);
  if (kret != KERN_SUCCESS) {
    return TASK_NULL;
  }

  return task;
}

thread_t MachInterface::getMachThread(ProcessThreadId const &ptid) {
  thread_t *thread_list;
  mach_msg_type_number_t thread_count;

  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return THREAD_NULL;

  kern_return_t kret = task_threads(task, &thread_list, &thread_count);
  if (kret != KERN_SUCCESS) {
    return THREAD_NULL;
  }

  // TODO: Find the specific thread by TID
  // For now, use first thread (needs improvement for multi-threading)
  thread_t thread = thread_list[0];

  vm_deallocate(mach_task_self(), (vm_address_t)thread_list,
                thread_count * sizeof(thread_t));

  return thread;
}

ErrorCode MachInterface::readMemory(ProcessThreadId const &ptid,
                                    Address const &address, void *buffer,
                                    size_t length, size_t *count) {
  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return kErrorProcessNotFound;

#if defined(__APPLE__)
  // Darwin: Use mach_vm_read_overwrite (64-bit VM API)
  mach_vm_size_t curr_bytes_read = 0;
  kern_return_t kret =
      mach_vm_read_overwrite((vm_map_t)task, address.value(), length,
                             (mach_vm_address_t)buffer, &curr_bytes_read);
  if (kret != KERN_SUCCESS)
    return kErrorInvalidAddress;

  if (count != nullptr) {
    *count = curr_bytes_read;
  }
#elif defined(__GNU__)
  // GNU/Hurd: Use vm_read (returns data that needs to be copied)
  vm_offset_t data;
  mach_msg_type_number_t data_count;
  kern_return_t kret = vm_read(task, (vm_address_t)address.value(), length,
                                &data, &data_count);
  if (kret != KERN_SUCCESS)
    return kErrorInvalidAddress;

  // Copy data to buffer
  size_t copy_size = (data_count < length) ? data_count : length;
  memcpy(buffer, (void *)data, copy_size);

  // Deallocate the data returned by vm_read
  vm_deallocate(mach_task_self(), data, data_count);

  if (count != nullptr) {
    *count = copy_size;
  }
#endif

  return kSuccess;
}

ErrorCode MachInterface::writeMemory(ProcessThreadId const &ptid,
                                     Address const &address,
                                     void const *buffer, size_t length,
                                     size_t *count) {
  ErrorCode error = kSuccess;

  task_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

#if defined(__APPLE__)
  // Darwin: Use mach_vm_* APIs with 64-bit addressing
  mach_msg_type_number_t infoCount = VM_REGION_SUBMAP_INFO_COUNT_64;
  vm_region_submap_info_data_64_t regionInfo;
  mach_vm_address_t start = address.value();
  mach_vm_size_t size;
  natural_t depth = 1024;

  kern_return_t kret =
      mach_vm_region_recurse(task, &start, &size, &depth,
                             (vm_region_recurse_info_t)&regionInfo, &infoCount);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  // The combination of VM_PROT_COPY/VM_PROT_READ is needed to override
  // write protection
  kret = mach_vm_protect(task, address.value(), length, FALSE,
                         VM_PROT_WRITE | VM_PROT_COPY | VM_PROT_READ);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  kret = mach_vm_write((vm_map_t)task, address.value(), (vm_offset_t)buffer,
                       length);
  if (kret != KERN_SUCCESS) {
    error = kErrorUnknown;
  } else if (count != nullptr) {
    *count = length;
  }

  // Restore original protection
  kret = mach_vm_protect(task, address.value(), length, FALSE,
                         regionInfo.protection);
  if (kret != KERN_SUCCESS && error == kSuccess) {
    error = kErrorUnknown;
  }

#elif defined(__GNU__)
  // GNU/Hurd: Use vm_* APIs with 32-bit addressing
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

  // Set write protection temporarily
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
#endif

  return error;
}

ErrorCode MachInterface::suspend(ProcessThreadId const &ptid) {
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

ErrorCode MachInterface::step(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo, int signal,
                               Address const &address) {
  // Neither Darwin nor GNU/Hurd have native single-step via Mach
  // This would need to be implemented using breakpoints
  return kErrorUnsupported;
}

ErrorCode MachInterface::resume(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo, int signal,
                                Address const &address) {
#if defined(__APPLE__)
  // Darwin: resume not directly supported via Mach
  return kErrorUnsupported;
#elif defined(__GNU__)
  // GNU/Hurd: Use thread_resume
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  kern_return_t kret = thread_resume(thread);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  return kSuccess;
#endif
}

ErrorCode MachInterface::getProcessMemoryRegion(ProcessId pid,
                                                Address const &address,
                                                MemoryRegionInfo &region) {
  task_t task = getMachTask(pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

#if defined(__APPLE__)
  // Darwin: Use mach_vm_region_recurse with 64-bit structures
  mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
  vm_region_submap_info_data_64_t regionInfo;
  mach_vm_address_t start = address.value();
  mach_vm_size_t size;
  natural_t depth = 1024;

  kern_return_t kret =
      mach_vm_region_recurse(task, &start, &size, &depth,
                             (vm_region_recurse_info_t)&regionInfo, &count);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "unable to retrieve region (0x%llx) info: %s",
           (unsigned long long)start, mach_error_string(kret));
    return kErrorUnknown;
  }

  region.start = start;
  region.length = size;

  if ((regionInfo.protection & VM_PROT_READ) == VM_PROT_READ)
    region.protection |= ds2::kProtectionRead;
  if ((regionInfo.protection & VM_PROT_WRITE) == VM_PROT_WRITE)
    region.protection |= ds2::kProtectionWrite;
  if ((regionInfo.protection & VM_PROT_EXECUTE) == VM_PROT_EXECUTE)
    region.protection |= ds2::kProtectionExecute;

#elif defined(__GNU__)
  // GNU/Hurd: Use vm_region with basic info
  vm_region_basic_info_data_t region_info;
  mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT;
  vm_address_t start = (vm_address_t)address.value();
  vm_size_t size;
  mach_port_t object_name;

  kern_return_t kret = vm_region(task, &start, &size, VM_REGION_BASIC_INFO,
                                 (vm_region_info_t)&region_info, &count,
                                 &object_name);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "unable to retrieve region (0x%llx) info",
           (unsigned long long)start);
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
#endif

  return kSuccess;
}

#if defined(__APPLE__)
// Darwin-specific implementations

ErrorCode MachInterface::getProcessDylbInfo(ProcessId pid, Address &address) {
  task_t task = getMachTask(pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  task_dyld_info_data_t dyldInfo;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  kern_return_t kret =
      task_info(task, TASK_DYLD_INFO, (task_info_t)&dyldInfo, &count);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  address = dyldInfo.all_image_info_addr;
  return kSuccess;
}

ErrorCode MachInterface::getThreadInfo(ProcessThreadId const &ptid,
                                       thread_basic_info_t info) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  unsigned int thread_info_count = THREAD_BASIC_INFO_COUNT;
  kern_return_t kret = thread_info(thread, THREAD_BASIC_INFO,
                                   (thread_info_t)info, &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

ErrorCode
MachInterface::getThreadIdentifierInfo(ProcessThreadId const &ptid,
                                       thread_identifier_info_data_t *threadInfo) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  unsigned int thread_info_count = THREAD_IDENTIFIER_INFO_COUNT;
  kern_return_t kret =
      thread_info(thread, THREAD_IDENTIFIER_INFO, (thread_info_t)threadInfo,
                  &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

#elif defined(__GNU__)
// GNU/Hurd-specific implementations

ErrorCode MachInterface::getThreadInfo(ProcessThreadId const &ptid, void *info) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  unsigned int thread_info_count = THREAD_BASIC_INFO_COUNT;
  kern_return_t kret = thread_info(thread, THREAD_BASIC_INFO,
                                   (thread_info_t)info, &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}
#endif

// CPU state functions are implemented in platform/architecture-specific files:
// - Darwin: Sources/Host/Darwin/X86_64/MachX86_64.cpp
// - Hurd: Sources/Host/Hurd/X86_64/MachX86_64.cpp, X86/MachX86.cpp
// These files provide the actual implementations for their respective platforms

} // namespace Mach
} // namespace Host
} // namespace ds2
