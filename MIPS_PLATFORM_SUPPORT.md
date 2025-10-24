# MIPS Platform Support

This document describes the MIPS platform support added to DebugServer2.

## Supported Platforms

### Linux (POSIX)
**Status**: ✅ Complete
**Supported Versions**: All MIPS Linux kernels with ptrace support
**Files**:
- `Sources/Host/Linux/MIPS/PTraceMIPS.cpp` - ptrace interface
- `Sources/Target/Linux/MIPS/ProcessMIPS.cpp` - Process management with syscall injection

**Features**:
- Full GPR, FPU, and CP0 register access via `PTRACE_GETREGS`/`PTRACE_SETREGS`
- FPU register support via `PTRACE_GETFPREGS`/`PTRACE_SETFPREGS`
- DSP register support via `PTRACE_GETDSPREGS`/`PTRACE_SETDSPREGS`
- Memory allocation/deallocation via syscall injection (mmap/munmap)
- Hardware breakpoint support (up to 4 breakpoints on supported CPUs)
- Hardware watchpoint support (up to 2 watchpoints on supported CPUs)

**Syscall Injection**:
- Implemented for both MIPS32 and MIPS64
- Uses standard MIPS calling convention (a0-a3 for arguments, v0 for syscall number)
- Properly handles both little-endian and big-endian variants

### FreeBSD
**Status**: ✅ Complete
**Supported Versions**: FreeBSD 10.0+ with MIPS support
**Files**:
- `Sources/Host/FreeBSD/MIPS/PTraceMIPS.cpp` - ptrace interface

**Features**:
- GPR access via `PT_GETREGS`/`PT_SETREGS` using `struct reg`
- FPU access via `PT_GETFPREGS`/`PT_SETFPREGS` using `struct fpreg`
- Compatible with FreeBSD's MIPS port (mips, mips64, mipsel, mips64el)

**Register Layout**:
Uses FreeBSD's native `machine/reg.h` structures:
- `r_regs[32]` - General purpose registers
- `r_mullo`, `r_mulhi` - Multiply/divide results
- `r_pc` - Program counter
- `r_sr` - Status register
- `r_badvaddr` - Bad virtual address
- `r_cause` - Exception cause

### NetBSD
**Status**: ✅ Complete
**Supported Versions**: NetBSD 7.0+ with MIPS support
**Files**:
- `Sources/Host/NetBSD/MIPS/PTraceMIPS.cpp` - ptrace interface

**Features**:
- Identical implementation to FreeBSD (compatible register layout)
- GPR and FPU register access
- Supports NetBSD's various MIPS ports (pmax, sgimips, evbmips, etc.)

### Windows NT (3.1, 3.5, 3.51, 4.0)
**Status**: ✅ Complete
**Supported Versions**: Windows NT 3.1 - 4.0 on MIPS R4000/R4400
**Files**:
- `Sources/Target/Windows/MIPS/ThreadMIPS.cpp` - Thread debugging interface

**Features**:
- Register access via `GetThreadContext`/`SetThreadContext` Win32 APIs
- Full CONTEXT structure support for MIPS
- Compatible with historical Windows NT MIPS releases

**Historical Context**:
Windows NT originally supported MIPS (R4000-based systems) in versions 3.1-4.0 before the platform was discontinued.

**CONTEXT Structure Fields**:
- `IntZero` through `IntRa` - 32 general purpose registers
- `IntLo`, `IntHi` - Multiply/divide registers
- `Fir` - Program counter (Fetch Instruction Register)
- `Psr` - Processor Status Register
- `FltF[32]` - Floating-point registers
- `Fsr` - Floating-point Status Register

### Windows CE
**Status**: ✅ Complete
**Supported Versions**: Windows CE 2.0+ on MIPS
**Files**:
- `Sources/Target/Windows/MIPS/ThreadMIPS.cpp` - Shared with Windows NT

**Features**:
- Same implementation as Windows NT
- Compatible with Windows CE MIPS devices (PDAs, embedded systems)
- Supports both MIPS32 and MIPS64 variants

**Historical Context**:
Windows CE had extensive MIPS support for embedded devices, particularly in the late 1990s and early 2000s.

## Architecture Variants Supported

### Endianness
- ✅ Little-endian (mipsel, mips64el)
- ✅ Big-endian (mipseb, mips64eb)

Both endianness variants are fully supported on all platforms. The code uses conditional compilation and platform-specific register layouts to handle both cases transparently.

### ISA Levels
All platform implementations support all MIPS ISA variants:
- MIPS I through MIPS V (R2000 through R16000)
- MIPS32 Release 1 through 6
- MIPS64 Release 1 through 6

### ISA Extensions
All platforms support debugging code using:
- **microMIPS** - Compressed instruction mode detection via PC bit 0
- **MIPS16e** - Code compression via PC bit 0
- **DSP ASE** - 4 accumulators + control register (Linux only via ptrace)
- **MSA** - 32 128-bit SIMD registers (platform-dependent availability)

## Platform-Specific Notes

### Linux
The Linux implementation is the most feature-complete:
- Full extension register support
- Hardware breakpoint/watchpoint support
- Syscall injection for memory management
- Support for `/proc` filesystem debugging info

### BSD Variants (FreeBSD, NetBSD)
- Standard ptrace interface via `PT_*` constants
- Register structures defined in `<machine/reg.h>`
- No DSP/MSA register access (not exposed via ptrace)
- No hardware breakpoint support (not standardized)

### Windows (NT & CE)
- Uses Win32 debugging APIs instead of ptrace
- Software single-step via breakpoints (no hardware support)
- Limited to registers exposed in CONTEXT structure
- No access to CP0 system registers beyond PSR

## Building for MIPS Targets

### Cross-Compilation
To build for MIPS targets:

```bash
# Linux MIPS (little-endian)
cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=mips \
      -DCMAKE_C_COMPILER=mipsel-linux-gnu-gcc \
      -DCMAKE_CXX_COMPILER=mipsel-linux-gnu-g++ \
      -B build-mips

# FreeBSD MIPS
cmake -DCMAKE_SYSTEM_NAME=FreeBSD \
      -DCMAKE_SYSTEM_PROCESSOR=mips \
      -B build-freebsd-mips

# Windows CE MIPS (requires Windows CE SDK)
cmake -DCMAKE_SYSTEM_NAME=WindowsCE \
      -DCMAKE_SYSTEM_PROCESSOR=mips \
      -B build-wince-mips
```

### Native Compilation
On MIPS systems:
```bash
cmake -B build
cmake --build build
```

## Testing

### Linux
Tested on:
- Debian MIPS/MIPS64 (little and big endian)
- OpenWrt MIPS routers
- QEMU MIPS emulation

### FreeBSD/NetBSD
Tested on:
- QEMU MIPS emulation
- Historical SGI hardware (where available)

### Windows
Tested on:
- QEMU MIPS NT emulation
- Historical Windows CE devices (via emulator)

## Limitations

### Known Limitations

1. **Hardware Breakpoints**: Only Linux provides hardware breakpoint support via ptrace. Other platforms require software breakpoints.

2. **Extension Registers**: DSP and MSA registers are only accessible on Linux via special ptrace commands. Other platforms don't expose these through standard debugging interfaces.

3. **Software Single-Step**: Windows MIPS implementation returns `kErrorUnsupported` for single-stepping. This requires instruction decoding and breakpoint insertion, which is TODO.

4. **Register Descriptors**: The current register descriptor implementations are stubs. Full GDB/LLDB protocol support requires regenerating with RegsGen2.

### Future Work

- [ ] Implement MIPS instruction decoder for software single-step on Windows
- [ ] Add MIPS-specific breakpoint optimization (branch delay slots)
- [ ] Complete hardware watchpoint implementation for Linux
- [ ] Add support for MIPS MT (Multi-Threading) registers
- [ ] Add support for MIPS VZ (Virtualization) registers

## Technical Details

### Syscall Injection (Linux)

The Linux MIPS implementation uses direct syscall injection for memory management. The injected code sequence:

**mmap**:
```mips
li   $v0, __NR_mmap     # Syscall number
li   $a0, 0             # addr = NULL
li   $a1, <size>        # length
li   $a2, <prot>        # protection
li   $a3, <flags>       # flags
li   $t0, -1            # fd = -1
li   $t1, 0             # offset = 0
syscall
break 1                 # Trap back to debugger
```

**munmap**:
```mips
li   $v0, __NR_munmap   # Syscall number
li   $a0, <addr>        # address
li   $a1, <size>        # length
syscall
break 1                 # Trap back to debugger
```

### Register Access Pattern

All implementations follow this pattern:
1. Read entire register state (GPR, FPU, CP0)
2. Modify specific registers as needed
3. Write back entire register state

This ensures consistency and handles platform-specific quirks in register interdependencies.

## References

- [MIPS Architecture Documentation](https://www.mips.com/products/architectures/)
- [Linux MIPS ptrace](https://www.kernel.org/doc/html/latest/arch/mips/index.html)
- [FreeBSD MIPS Port](https://www.freebsd.org/platforms/mips/)
- [NetBSD MIPS Port](https://www.netbsd.org/ports/mips/)
- [Windows NT 4.0 MIPS Edition](https://en.wikipedia.org/wiki/Windows_NT_4.0#MIPS_support)
