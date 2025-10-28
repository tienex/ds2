# Unified Debugging Architecture in ds2

This document describes how ds2 unifies debugging across different operating system families through abstraction layers.

## Overview

The ds2 codebase already implements extensive unification for similar operating systems through base classes and inheritance. This reduces code duplication while allowing platform-specific customization.

## Mach-Based Systems Unification

### Unified Interface: `Host/Mach/Mach.h`

All Mach-based systems share a common debugging interface through `MachInterface`:

**Supported Platforms:**
- **Darwin (macOS, iOS, watchOS, tvOS)** - XNU Mach 3.0
- **GNU/Hurd** - GNU Mach 1.x (based on Mach 3.0)
- **OSF/1 (Digital UNIX / Tru64)** - Mach 2.5/3.0

**Implementation Strategy:**

```cpp
// Host/Mach/Mach.h
namespace ds2::Host::Mach {
  class MachInterface {
    // Unified API for all Mach platforms
    ErrorCode readMemory(ProcessThreadId const &ptid, ...);
    ErrorCode writeMem ory(ProcessThreadId const &ptid, ...);
    ErrorCode readCPUState(ProcessThreadId const &ptid, ...);
    ErrorCode writeCPUState(ProcessThreadId const &ptid, ...);
    // ... more common operations
  };
}
```

**Platform-Specific Headers:**

```cpp
// Host/Darwin/Mach.h
namespace ds2::Host::Darwin {
  using Mach = ::ds2::Host::Mach::MachInterface;  // Use unified interface
}

// Host/Hurd/Mach.h
namespace ds2::Host::Hurd {
  using Mach = ::ds2::Host::Mach::MachInterface;  // Use unified interface
}

// Host/OSF1/Mach.h
namespace ds2::Host::OSF1 {
  using Mach = ::ds2::Host::Mach::MachInterface;  // Use unified interface
}
```

**Compile-Time Platform Detection:**

The unified `Mach.h` uses preprocessor directives to include the correct platform headers:

```cpp
#if defined(__APPLE__)
  // Darwin (XNU Mach)
  #include <mach/mach.h>
#elif defined(__GNU__)
  // GNU/Hurd (GNU Mach)
  #include <mach.h>
#elif defined(__osf__) || defined(__digital__)
  // OSF/1 (Digital UNIX / Tru64)
  #include <mach.h>
#endif
```

### Systems NOT Unified with Mach

**NeXT/Debug.h** - NeXTSTEP, OpenStep, Rhapsody
- Has its own Mach implementation with custom types and APIs
- Could potentially be migrated to use `Mach::MachInterface`
- Kept separate for historical compatibility

**MachTen/Debug.h** - Unix environment for classic Mac OS
- Based on Mach 2.5/2.6 microkernel but runs on classic Mac OS
- Uses POSIX/PTrace in addition to Mach-specific operations
- Special case: Mach hosted on non-Mach OS (System 7, Mac OS 8/9)
- Requires separate implementation due to unique environment

**MacOS/Debug.h** - Classic Mac OS (System 1-9)
- NOT Mach-based - uses MacsBug protocol and Toolbox traps
- Completely different architecture (cooperative multitasking, 68k/PowerPC)
- Should not be unified with Mach systems

## BSD Systems Unification

### Unified Interface: `Host/POSIX/PTrace.h`

All BSD variants share a common ptrace-based debugging interface:

**Supported Platforms:**
- **FreeBSD** - `Host/FreeBSD/PTrace.h`
- **OpenBSD** - `Host/OpenBSD/PTrace.h`
- **DragonFly BSD** - `Host/DragonFly/PTrace.h`
- **4.3BSD-Tahoe** - `Host/BSD_Tahoe/Debug.h`

**Implementation Strategy:**

```cpp
// Host/POSIX/PTrace.h
namespace ds2::Host::POSIX {
  class PTrace {
    virtual ErrorCode wait(ProcessThreadId const &ptid, int *status = nullptr);
    virtual ErrorCode traceMe(bool disableASLR);
    virtual ErrorCode traceThat(ProcessId pid) = 0;  // Platform-specific
    virtual ErrorCode attach(ProcessId pid);
    virtual ErrorCode detach(ProcessId pid);
    virtual ErrorCode readMemory(...) = 0;  // Platform-specific
    virtual ErrorCode writeMemory(...) = 0;  // Platform-specific
    virtual ErrorCode readCPUState(...) = 0;  // Platform-specific
    virtual ErrorCode writeCPUState(...) = 0;  // Platform-specific
    // ... more operations
  };
}
```

**Platform-Specific Implementations:**

```cpp
// Host/FreeBSD/PTrace.h
namespace ds2::Host::FreeBSD {
  class PTrace : public POSIX::PTrace {
    ErrorCode traceThat(ProcessId pid) override;
    ErrorCode readMemory(...) override;
    ErrorCode writeMemory(...) override;
    ErrorCode readCPUState(...) override;
    ErrorCode writeCPUState(...) override;
    ErrorCode getLwpInfo(...);  // FreeBSD-specific: lightweight process info
    // ... FreeBSD-specific overrides
  };
}

// Host/OpenBSD/PTrace.h
namespace ds2::Host::OpenBSD {
  class PTrace : public POSIX::PTrace {
    // Same interface as FreeBSD, but OpenBSD-specific implementation
    ErrorCode traceThat(ProcessId pid) override;
    ErrorCode readMemory(...) override;
    ErrorCode writeMemory(...) override;
    ErrorCode readCPUState(...) override;
    ErrorCode writeCPUState(...) override;
  };
}

// Host/DragonFly/PTrace.h
namespace ds2::Host::DragonFly {
  class PTrace : public POSIX::PTrace {
    // Same interface as FreeBSD/OpenBSD
    ErrorCode traceThat(ProcessId pid) override;
    ErrorCode readMemory(...) override;
    ErrorCode writeMemory(...) override;
    ErrorCode readCPUState(...) override;
    ErrorCode writeCPUState(...) override;
  };
}
```

**Differences Between BSD Variants:**

- **FreeBSD**: Adds `getLwpInfo()` for lightweight process (thread) information
- **OpenBSD**: Standard POSIX ptrace interface
- **DragonFly**: Standard POSIX ptrace interface (similar to FreeBSD)
- All override architecture-specific register operations

## System V (SVR4) Unification

### Unified Interface: `Host/SystemV/ProcFS.h`

System V variants that use `/proc` filesystem for debugging share a common interface:

**Supported Platforms:**
- **AIX** - IBM Unix (POWER)
- **HP-UX** - Hewlett-Packard Unix (PA-RISC, Itanium)
- **IRIX** - SGI Unix (MIPS)
- **Solaris** - Sun/Oracle Unix (SPARC, x86)
- **OpenServer** - SCO Unix (x86)
- **UnixWare** - SCO Unix (x86)

**Implementation Strategy:**

```cpp
// Host/SystemV/ProcFS.h
#if defined(OS_SYSV)
namespace ds2::Host::SystemV {
  class ProcFS {
    // Common procfs operations
    virtual ErrorCode wait(ProcessThreadId const &ptid, int *status = nullptr);
    virtual ErrorCode traceMe(bool disableASLR);
    virtual ErrorCode traceThat(ProcessId pid) = 0;
    virtual ErrorCode attach(ProcessId pid);
    virtual ErrorCode detach(ProcessId pid);
    virtual ErrorCode readMemory(...) = 0;
    virtual ErrorCode writeMemory(...) = 0;
    virtual ErrorCode readCPUState(...) = 0;
    virtual ErrorCode writeCPUState(...) = 0;

  protected:
    // Helper functions for procfs operations
    static int openProc(ProcessId pid, const char *file, int flags);
    static ErrorCode controlProc(int ctlfd, long cmd, void *data = nullptr, size_t dataSize = 0);
    static ErrorCode waitProc(int ctlfd);

    // Platform-specific control operations
    virtual long wrapProcfsControl(int ctlfd, long cmd, void *data) = 0;
  };
}
#endif
```

**Platform-Specific Implementations:**

```cpp
// Host/AIX/ProcFS.h
#if defined(OS_AIX)
namespace ds2::Host::AIX {
  class ProcFS : public SystemV::ProcFS {
    ErrorCode traceThat(ProcessId pid) override;
    ErrorCode readMemory(...) override;
    ErrorCode writeMemory(...) override;
    ErrorCode readCPUState(...) override;
    ErrorCode writeCPUState(...) override;

  protected:
    long wrapProcfsControl(int ctlfd, long cmd, void *data) override;
  };
}
#endif
```

**Key Differences from ptrace-based Systems:**

System V `/proc` debugging:
- Uses file operations (`open()`, `read()`, `write()`, `ioctl()`) instead of `ptrace()` syscall
- `/proc/<pid>/ctl` - Control file for process control commands
- `/proc/<pid>/as` - Address space file for memory operations
- `/proc/<pid>/status` - Process status information
- More consistent API across platforms than ptrace

### Systems NOT Unified with System V

**XENIX** - `Host/XENIX/Debug.h`
- Microsoft XENIX (System V derivative)
- Uses `ptrace()` instead of `/proc` filesystem
- Uses x.out executable format (not ELF or COFF)
- Significantly different from modern System V variants
- Kept separate due to different debugging model

## Windows Systems

### Modern Windows: `Host/Windows/`

**Platform:** Windows NT, 2000, XP, Vista, 7, 8, 10, 11, Windows Server

**Debugging API:** Win32 Debugging API
- `CreateProcess()` with `DEBUG_PROCESS` flag
- `DebugActiveProcess()` for attaching
- `WaitForDebugEvent()` / `WaitForDebugEventEx()`
- `ContinueDebugEvent()`
- `ReadProcessMemory()` / `WriteProcessMemory()`
- `GetThreadContext()` / `SetThreadContext()`

**Files:**
- `Host/Windows/ExtraWrappers.h` - API wrappers and compatibility shims
- `Host/Windows/ProcessSpawner.h` - Process creation and debugging setup

### Windows 3.x: `Host/Windows3x/`

**Platform:** Windows 3.0, 3.1, 3.11 (16-bit)

**Debugging API:** TOOLHELP.DLL
- Task Database (TDB) - task control blocks
- Module Database (MDB) - module information
- Segmented memory model (selector:offset addressing)
- Real mode, Standard mode (286), Enhanced mode (386)

**Files:**
- `Host/Windows3x/Debug.h` - Complete TOOLHELP.DLL debugging interface

### Why Windows Cannot Be Unified

**Fundamental Differences:**

| Aspect | Windows NT+ (32/64-bit) | Windows 3.x (16-bit) |
|--------|-------------------------|----------------------|
| Architecture | Win32 API, flat memory | TOOLHELP.DLL, segmented memory |
| Addressing | 32/64-bit linear | 16-bit selector:offset |
| Process Model | Preemptive multitasking | Cooperative multitasking |
| Memory Model | Virtual memory, paging | Segmented, real/protected mode |
| Executable Format | PE (Portable Executable) | NE (New Executable) |
| API Style | Win32 functions | TOOLHELP interrupts (Int 4Eh) |

**Conclusion:** These are fundamentally different operating systems that happen to share the "Windows" name. Unification is not possible or desirable.

## Systems That Should Remain Separate

### By Architecture

1. **DOS** (`Host/DOS/Debug.h`)
   - Real-mode x86, no memory protection, no multitasking
   - Uses DEBUG.COM style debugging and INT 21h calls

2. **ELKS** (`Host/ELKS/Debug.h`)
   - Embeddable Linux Kernel Subset for 8086/286
   - Minimal debugging facilities, custom ptrace-like interface

3. **OS/2 16-bit** (`Host/OS2_16/Debug.h`)
   - OS/2 1.x, segmented memory, different API than OS/2 2.x+

4. **Classic Mac OS** (`Host/MacOS/Debug.h`)
   - System 1-9, MacsBug protocol, Toolbox traps
   - NOT Mach-based (that's macOS X/Darwin)

5. **BeOS** (`Host/BeOS/`)
   - Unique microkernel architecture
   - Different threading model (pervasive multithreading)

6. **Plan 9** (`Host/Plan9/`)
   - Unique debugging model via `/proc` but very different from Unix
   - Process-per-thread model

7. **QNX** (`Host/QNX/`)
   - Microkernel RTOS with unique IPC model
   - procfs-based but QNX-specific

8. **Coherent** (`Host/Coherent/Debug.h`)
   - Unix clone with unique implementation details

9. **MINIX** (`Host/MINIX/Debug.h`)
   - Microkernel educational OS with custom debugging

10. **NetWare** (`Host/NetWare/Debug.h`)
    - Novell network OS, NLM-based, unique architecture

11. **OpenVMS** (`Host/OpenVMS/Debug.h`)
    - VAX/VMS descendant, completely different debugging model

12. **UNICOS / UNICOS/mk** (`Host/UNICOS/`, `Host/UNICOSmk/`)
    - Cray supercomputer OS, word-addressed vector architecture
    - Massively parallel (T3D/T3E with up to 2176 PEs)

13. **Fuchsia** (`Host/Fuchsia/Debug.h`)
    - Google's capability-based OS with Zircon microkernel
    - Handle-based object model, FIDL IPC

14. **Sprite** (`Host/Sprite/Debug.h`)
    - UC Berkeley distributed OS with process migration

15. **Xinu / XinuMT** (`Host/Xinu/`, `Host/XinuMT/`)
    - Educational OS with unique threading model

16. **WASM** (`Host/WASM/Debug.h`)
    - WebAssembly virtual machine, not a real OS

### By Historical Significance

1. **Ultrix** (`Host/Ultrix/Debug.h`)
   - DEC Unix (Ultrix-32), VAX and MIPS
   - Historical System V/BSD hybrid

2. **4.3BSD-Tahoe** (`Host/BSD_Tahoe/Debug.h`)
   - First multi-architecture BSD
   - Historical significance (CCI Power 6/32 / Tahoe architecture)

## Unification Recommendations

### Already Well-Unified ✅

1. **Mach systems** - Unified via `Host/Mach/Mach.h`
   - Darwin, GNU/Hurd, OSF/1 all use this

2. **BSD systems** - Unified via `Host/POSIX/PTrace.h`
   - FreeBSD, OpenBSD, DragonFly all inherit from this

3. **System V systems** - Unified via `Host/SystemV/ProcFS.h`
   - AIX and other procfs-based SVR4 variants use this

### Potential Improvements 🔧

1. **NeXT/Debug.h** could potentially be refactored to use `Host/Mach/Mach.h`
   - Would require careful migration to ensure compatibility
   - NeXTSTEP, OpenStep, Rhapsody, Mac OS X Server would benefit
   - Low priority since these are historical platforms

2. **NetBSD support** could be added
   - Would inherit from `Host/POSIX/PTrace.h` like other BSDs
   - Currently not implemented

### Should NOT Be Unified ⛔

1. **Windows 3.x vs Windows NT+** - Fundamentally different architectures
2. **Classic Mac OS vs Darwin** - Completely different OS families
3. **DOS vs any Unix** - No memory protection vs protected mode
4. **Real-mode vs protected-mode** systems
5. **Embedded/RTOS vs general-purpose** OS (QNX, MINIX, Xinu)

## Architecture Benefits

### Code Reuse

- **Mach**: Common Mach task/thread/port operations shared across Darwin, Hurd, OSF/1
- **BSD**: Common ptrace operations shared across FreeBSD, OpenBSD, DragonFly
- **System V**: Common procfs operations shared across AIX, HP-UX, IRIX, Solaris

### Platform-Specific Customization

Each platform can:
1. Override virtual functions for platform-specific behavior
2. Add platform-specific methods (e.g., FreeBSD's `getLwpInfo()`)
3. Customize architecture-specific register operations

### Compile-Time Selection

Preprocessor directives ensure:
1. Only relevant code is compiled for each platform
2. No runtime overhead from abstraction
3. Type-safe platform-specific APIs

## Conclusion

The ds2 codebase already implements extensive unification for similar operating systems:

- ✅ **Mach systems are unified** through `Host/Mach/Mach.h`
- ✅ **BSD systems are unified** through `Host/POSIX/PTrace.h`
- ✅ **System V systems are unified** through `Host/SystemV/ProcFS.h`
- ⛔ **Windows cannot be unified** (fundamentally different architectures)

The current architecture provides:
- **Code reuse** through base classes and inheritance
- **Platform flexibility** through virtual function overrides
- **Type safety** through compile-time selection
- **Zero runtime overhead** through inlining and compile-time dispatch

Further unification is not recommended as most similar systems are already unified, and remaining systems have fundamental architectural differences that make unification impractical or counterproductive.
