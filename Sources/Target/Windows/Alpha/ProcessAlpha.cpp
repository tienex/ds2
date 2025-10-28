//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// Windows 2000 Alpha64 (AXP64) Process - Memory allocation
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

namespace ds2 {
namespace Target {
namespace Windows {

//
// Windows 2000 AXP64 Memory Allocation
//
// On Windows, we use VirtualAllocEx/VirtualFreeEx instead of
// syscall injection. This is the standard Windows API approach.
//

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr)
    return kErrorInvalidArgument;

  // Map ds2 protection flags to Windows protection flags
  DWORD winProtection = PAGE_NOACCESS;
  if (protection & kProtectionRead) {
    if (protection & kProtectionWrite) {
      winProtection = (protection & kProtectionExecute) ? PAGE_EXECUTE_READWRITE
                                                        : PAGE_READWRITE;
    } else {
      winProtection = (protection & kProtectionExecute) ? PAGE_EXECUTE_READ
                                                        : PAGE_READONLY;
    }
  } else if (protection & kProtectionExecute) {
    winProtection = PAGE_EXECUTE;
  }

  // Allocate memory in the target process
  LPVOID addr = VirtualAllocEx(_handle, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                               winProtection);

  if (addr == nullptr) {
    DS2LOG(Error, "VirtualAllocEx failed: %lu", GetLastError());
    return kErrorNoMemory;
  }

  *address = reinterpret_cast<uint64_t>(addr);

  DS2LOG(Debug, "allocated %zu bytes at %#" PRIx64 " with protection %#x",
         size, *address, protection);

  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  LPVOID addr = reinterpret_cast<LPVOID>(address);

  if (!VirtualFreeEx(_handle, addr, 0, MEM_RELEASE)) {
    DS2LOG(Error, "VirtualFreeEx failed: %lu", GetLastError());
    return kErrorInvalidArgument;
  }

  DS2LOG(Debug, "deallocated memory at %#" PRIx64, address);

  return kSuccess;
}

} // namespace Windows
} // namespace Target
} // namespace ds2
