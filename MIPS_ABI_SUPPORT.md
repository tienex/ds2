# MIPS ABI Support in ds2

This document describes the MIPS Application Binary Interface (ABI) support in ds2.

## Overview

MIPS architectures support multiple ABIs that define calling conventions, register usage, and system call interfaces. ds2 supports all major MIPS ABIs:

- **O32** - Original 32-bit ABI
- **N32** - New 32-bit ABI for MIPS64
- **N64** - New 64-bit ABI
- **O64** - Original 64-bit ABI (rarely used)
- **EABI** - Embedded ABI
- **NUBI** - New ABI (experimental)

## ABI Comparison

| Feature | O32 | N32 | N64 | O64 | EABI |
|---------|-----|-----|-----|-----|------|
| Pointer Size | 32-bit | 32-bit | 64-bit | 64-bit | 32/64-bit |
| Register Size | 32-bit | 64-bit | 64-bit | 64-bit | 32/64-bit |
| Integer Args | 4 (a0-a3) | 8 (a0-a7) | 8 (a0-a7) | 4 (a0-a3) | 8 (a0-a7) |
| FP Args | 2 | 8 | 8 | 2 | 8 |
| Stack Align | 8 bytes | 16 bytes | 16 bytes | 16 bytes | 8 bytes |
| Stack Args | Args 5+ | Args 9+ | Args 9+ | Args 5+ | Args 9+ |

## Register Conventions

### General Purpose Registers

All MIPS ABIs use the following register naming:

```
$0  (zero) - Always zero
$1  (at)   - Assembler temporary
$2-$3      - Return values (v0-v1)
$4-$7      - First 4 arguments (a0-a3) [O32, O64]
$4-$11     - First 8 arguments (a0-a7) [N32, N64, EABI]
$8-$15     - Temporaries (t0-t7) [O32, O64]
$12-$15    - Temporaries (t0-t3) [N32, N64]
$16-$23    - Saved registers (s0-s7)
$24-$25    - Temporaries (t8-t9)
$26-$27    - Kernel registers (k0-k1)
$28        - Global pointer (gp)
$29        - Stack pointer (sp)
$30        - Frame pointer (s8/fp)
$31        - Return address (ra)
```

### O32 ABI Details

The O32 ABI is the traditional 32-bit MIPS ABI:

- **Argument Registers**: a0-a3 ($4-$7)
- **Stack Arguments**: Arguments 5+ passed on stack
- **Stack Frame**: 16-byte argument save area for callee
- **Return Values**: Integer in v0-v1, float in $f0-$f1
- **Syscall Numbers**: Base 4000 (e.g., mmap=4090, munmap=4091)

**Syscall Convention**:
```assembly
li   $v0, syscall_number  # Syscall number in $v0
li   $a0, arg1            # Args 1-4 in a0-a3
li   $a1, arg2
li   $a2, arg3
li   $a3, arg4
# Args 5-6 on stack at 16($sp), 20($sp)
syscall
```

### N32 ABI Details

The N32 ABI provides a 32-bit pointer model on MIPS64 hardware:

- **Argument Registers**: a0-a7 ($4-$11)
- **All arguments in registers** (for syscalls with ≤8 args)
- **64-bit registers** with 32-bit pointers
- **Return Values**: Integer in v0-v1, float in $f0-$f1
- **Syscall Numbers**: Base 6000 (e.g., mmap=6009, munmap=6011)
- **Stack Alignment**: 16 bytes

**Syscall Convention**:
```assembly
li   $v0, syscall_number  # Syscall number in $v0
li   $a0, arg1            # Args 1-8 in a0-a7
li   $a1, arg2
li   $a2, arg3
li   $a3, arg4
li   $a4, arg5
li   $a5, arg6
li   $a6, arg7
li   $a7, arg8
syscall
```

### N64 ABI Details

The N64 ABI is the standard 64-bit MIPS ABI:

- **Argument Registers**: a0-a7 ($4-$11)
- **All arguments in registers** (for syscalls with ≤8 args)
- **64-bit registers** with 64-bit pointers
- **Return Values**: Integer in v0-v1, float in $f0-$f1
- **Syscall Numbers**: Base 5000 (e.g., mmap=5009, munmap=5011)
- **Stack Alignment**: 16 bytes

Same calling convention as N32 but with 64-bit pointers.

### EABI Details

The Embedded ABI is optimized for embedded systems:

- **Argument Registers**: a0-a7 ($4-$11)
- **Optimized for code size**
- **Flexible register usage**
- **May vary by implementation**

### O64 ABI Details

The O64 ABI is the original 64-bit ABI (now obsolete):

- **Argument Registers**: a0-a3 ($4-$7) like O32
- **64-bit registers** with 64-bit pointers
- **Mostly replaced by N64**
- **Rare in modern systems**

## ABI Detection

ds2 detects the ABI using compiler-defined macros:

```cpp
#if defined(_ABIO32)
  // O32 ABI
#elif defined(_ABIN32)
  // N32 ABI
#elif defined(_ABI64)
  // N64 ABI
#elif defined(_ABIO64)
  // O64 ABI
#elif defined(__mips_eabi)
  // EABI
#endif
```

Runtime ABI detection is available via:
```cpp
Architecture::MIPS::ABI abi = Architecture::MIPS::DetectABI();
const char *name = Architecture::MIPS::GetABIName(abi);
```

Or through the Platform API:
```cpp
const char *abi = Host::Platform::GetMIPSABI();
```

## Syscall Injection

ds2 injects code to perform syscalls (e.g., mmap/munmap) with ABI-specific conventions:

### O32 Syscall Injection

For O32, arguments 5-6 must be passed on the stack:

```assembly
addiu $sp, $sp, -32      # Allocate stack frame
sw    $ra, 28($sp)       # Save return address
li    $v0, 4090          # __NR_mmap (O32)
li    $a0, 0             # addr = NULL
li    $a1, size          # length
li    $a2, prot          # protection
li    $a3, flags         # flags
li    $t0, -1            # fd = -1
sw    $t0, 16($sp)       # Store arg5 on stack
li    $t1, 0             # offset = 0
sw    $t1, 20($sp)       # Store arg6 on stack
syscall
lw    $ra, 28($sp)       # Restore return address
addiu $sp, $sp, 32       # Deallocate stack
break 1
```

### N32/N64 Syscall Injection

For N32/N64, all 6 arguments fit in registers:

```assembly
li   $v0, 6009           # __NR_mmap (N32) or 5009 (N64)
li   $a0, 0              # addr = NULL
li   $a1, size           # length
li   $a2, prot           # protection
li   $a3, flags          # flags
li   $a4, -1             # fd = -1
li   $a5, 0              # offset = 0
syscall
break 1
```

## Syscall Numbers

Different ABIs use different syscall number ranges:

### O32 (Base 4000)
- `__NR_mmap = 4090`
- `__NR_munmap = 4091`
- `__NR_mprotect = 4125`

### N32 (Base 6000)
- `__NR_mmap = 6009`
- `__NR_munmap = 6011`
- `__NR_mprotect = 6010`

### N64 (Base 5000)
- `__NR_mmap = 5009`
- `__NR_munmap = 5011`
- `__NR_mprotect = 5010`

## Implementation Files

### ABI Detection and Definitions
- `Headers/DebugServer2/Architecture/MIPS/ABI.h`
  - Defines ABI enum and structures
  - Provides ABIInfo with calling convention details
  - DetectABI() function for compile-time detection

### Platform Integration
- `Headers/DebugServer2/Host/Platform.h`
  - GetMIPSABI() function declaration
- `Sources/Host/Common/Platform.cpp`
  - GetMIPSABI() implementation

### Syscall Injection
- `Sources/Target/Linux/MIPS/ProcessMIPS.cpp`
  - O32, N32, N64 syscall injection code
  - ABI-aware mmap/munmap implementations
- `Sources/Target/Linux/MIPS64/ProcessMIPS64.cpp`
  - MIPS64-specific syscall injection

### CPU State Documentation
- `Headers/DebugServer2/Architecture/MIPS/CPUState.h`
  - Detailed ABI register conventions for MIPS32
- `Headers/DebugServer2/Architecture/MIPS64/CPUState.h`
  - Detailed ABI register conventions for MIPS64

## Testing

To test ABI support:

1. **Compile ds2 for target ABI**:
   ```bash
   # O32
   cmake -DCMAKE_CXX_FLAGS="-mabi=32" ..

   # N32
   cmake -DCMAKE_CXX_FLAGS="-mabi=n32" ..

   # N64
   cmake -DCMAKE_CXX_FLAGS="-mabi=64" ..
   ```

2. **Check ABI detection**:
   ```bash
   ./ds2 --version  # Should show ABI info
   ```

3. **Test syscall injection**:
   - Attach to a process
   - Execute `allocate` command
   - Verify correct syscall numbers used

## References

- [MIPS O32 ABI Specification](https://refspecs.linuxfoundation.org/elf/mipsabi.pdf)
- [MIPS N32/N64 ABI Specification](https://www.linux-mips.org/pub/linux/mips/doc/ABI/MIPS-N32-N64-Calling-Convention.pdf)
- [MIPS EABI Documentation](https://www.mips.com/products/architectures/ase/)
- [Linux MIPS Syscall Numbers](https://github.com/torvalds/linux/blob/master/arch/mips/include/uapi/asm/unistd.h)

## Notes

- **N32 is recommended** for MIPS64 systems when 32-bit pointers are sufficient
- **N64 should be used** when 64-bit addressing is required
- **O32 is legacy** but still widely used on MIPS32 systems
- **EABI is for embedded** systems only
- **O64 and NUBI are rare** and may have limited support

## Future Work

- Runtime ABI detection from ELF headers
- Support for mixed-ABI debugging
- ABI-specific register formatting in GDB protocol
- Automatic ABI detection from target process
