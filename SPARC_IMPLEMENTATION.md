# SPARC/SPARC64 Implementation Notes

## Architecture Support

### SPARC (32-bit)
- Headers/DebugServer2/Architecture/SPARC/CPUState.h
- 32 general-purpose registers with register windows
- PSR, WIM, TBR control registers
- FPU with 32 single-precision registers

### SPARC64 (64-bit - SPARCv9)
- Headers/DebugServer2/Architecture/SPARC64/CPUState.h
- 64-bit register extension
- Enhanced trap handling, multiple trap levels
- Extended FPU (32 double-precision registers)

## Platform Implementations

### SPARC Platforms:
- SunOS SPARC: Sources/Host/SunOS/SPARC/ (ptrace)
- Solaris SPARC: Sources/Host/Solaris/SPARC/ (procfs /proc)
- Linux SPARC: Sources/Host/Linux/SPARC/ (ptrace GETREGSET)
- NetBSD SPARC: Sources/Host/NetBSD/SPARC/ (PT_GETREGS)
- OpenBSD SPARC: Sources/Host/OpenBSD/SPARC/ (PT_GETREGS)

### SPARC64 Platforms:
- Solaris SPARC64: Sources/Host/Solaris/SPARC64/ (procfs)
- Linux SPARC64: Sources/Host/Linux/SPARC64/ (ptrace)
- FreeBSD SPARC64: Sources/Host/FreeBSD/SPARC64/ (PT_GETREGS)
- NetBSD SPARC64: Sources/Host/NetBSD/SPARC64/ (PT_GETREGS)
- OpenBSD SPARC64: Sources/Host/OpenBSD/SPARC64/ (PT_GETREGS)

## x32 ABI Support
- Linux x32: Sources/Host/Linux/X32/
- x86-64 ISA with 32-bit pointers (ILP32)
- Uses x86_64 register state
