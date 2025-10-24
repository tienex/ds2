# Multi-Architecture Linux Support

## Overview

This implementation adds Linux debugging support for 23 additional CPU architectures, covering active modern processors, embedded systems, DSPs, and legacy/historical architectures.

## Architecture Categories

### Active / Production Architectures

#### S390 / S390X (IBM System z Mainframes)
- **S390**: 31-bit IBM mainframe (ESA/390)
- **S390X**: 64-bit z/Architecture (System z9-z16)
- **Features**: 16 general registers, PSW, access registers, FPU, vector facilities
- **Use**: Enterprise mainframes, banking, insurance, government
- **Status**: Actively developed, latest z16 (2022)

#### Hexagon (Qualcomm)
- **Type**: VLIW DSP processor
- **Features**: 32 general registers, hardware loops, predicate registers
- **Use**: Qualcomm Snapdragon SoCs (modems, audio, sensors)
- **Status**: Active in mobile devices

#### ARC (Synopsys)
- **Type**: Configurable 32-bit RISC
- **Features**: 32 registers, hardware loops, DSP extensions
- **Use**: Embedded systems, IoT, storage controllers
- **Status**: Active, widely licensed

#### Xtensa (Tensilica/Cadence)
- **Type**: Configurable RISC processor
- **Features**: 16 address registers, windowed registers, MAC units
- **Use**: ESP32/ESP8266, audio DSPs, networking
- **Status**: Very active in IoT/embedded

#### OpenRISC (or1k)
- **Type**: Open-source RISC
- **Features**: 32 registers, r0=0, r1=sp, r2=fp, r9=lr
- **Use**: FPGA soft cores, academic, embedded
- **Status**: Open-source, actively maintained

#### MicroBlaze (Xilinx)
- **Type**: Soft processor for FPGAs
- **Features**: 32 registers, configurable pipeline, FPU optional
- **Use**: Xilinx FPGAs (Spartan, Virtex, Zynq)
- **Status**: Active in FPGA designs

#### C-SKY (Alibaba/T-Head)
- **Type**: Chinese RISC processor
- **Features**: 32 registers, DSP extensions, FPU/vector units
- **Use**: Embedded Linux devices, IoT, Chinese market
- **Status**: Recently added to Linux mainline (2018+)

### Specialty / DSP Architectures

#### C6X (Texas Instruments C6000)
- **Type**: DSP with VLIW architecture
- **Features**: Dual register files (A/B sides), 8 functional units
- **Use**: Telecom, industrial DSP, BeagleBone AI
- **Status**: Legacy but still deployed

#### Blackfin (Analog Devices)
- **Type**: 16/32-bit DSP
- **Features**: Dual MAC units, video processing
- **Use**: Audio/video processing, industrial control
- **Status**: Discontinued (2017), legacy support

### Embedded / Microcontroller Architectures

#### H8/300 (Renesas/Hitachi)
- **Type**: 8/16/32-bit microcontroller
- **Features**: 8 extended registers (ER0-ER7)
- **Use**: Automotive, industrial embedded
- **Status**: Legacy, replaced by RX/RL78

#### AVR32 (Atmel)
- **Type**: 32-bit RISC microcontroller
- **Features**: 16 registers, DSP extensions
- **Use**: Embedded systems, replaced by ARM
- **Status**: Discontinued, removed from Linux 4.12

### Chinese / Regional Architectures

#### Elbrus2K (e2k)
- **Type**: Russian VLIW processor
- **Features**: 64-bit, predicated execution, binary translation (x86/ARM)
- **Use**: Russian government/military systems
- **Status**: Active in Russia

#### Unicore32
- **Type**: Chinese 32-bit RISC
- **Features**: ARM-like instruction set
- **Use**: Chinese embedded systems
- **Status**: Removed from Linux 5.9

#### NDS32 (Andes Technology)
- **Type**: Taiwanese RISC processor
- **Features**: 32 registers, DSP extensions
- **Use**: Storage, networking, multimedia SoCs
- **Status**: Removed from Linux 5.17

#### Sunway (sw64)
- **Type**: Chinese 64-bit processor
- **Features**: Used in Sunway TaihuLight supercomputer
- **Status**: Limited information, specialized use

### Legacy / Historical Architectures

#### CRIS (Axis ETRAX)
- **Features**: Network cameras, NAS devices
- **Status**: Removed from Linux 5.4

#### FRV (Fujitsu FR-V)
- **Features**: 64 general registers, VLIW
- **Status**: Removed from Linux 5.7

#### META (Imagination)
- **Features**: Thread-based DSP
- **Status**: Removed from Linux 4.17

#### SCORE (Sunplus S+core)
- **Features**: 32 registers
- **Status**: Removed from Linux 5.0

#### M32R (Renesas)
- **Features**: 16 registers, 32-bit RISC
- **Status**: Removed from Linux 5.1

#### MN10300 (Panasonic)
- **Features**: 4 data + 4 address registers
- **Status**: Removed from Linux 4.17

#### TILE (Tilera)
- **Features**: Many-core mesh architecture, 56 64-bit registers
- **Use**: Networking, servers
- **Status**: Removed from Linux 4.17 (acquired by Mellanox/NVIDIA)

#### Transmeta Crusoe
- **Features**: VLIW with x86 code morphing
- **Status**: Historical, company defunct

## Implementation Structure

### Architecture Headers
- `Headers/DebugServer2/Architecture/{ARCH}/CPUState.h`
- Defines complete register state for each architecture

### Linux Ptrace Implementations
- `Sources/Host/Linux/{ARCH}/PTrace{ARCH}.cpp`
- Uses PTRACE_GETREGSET/PTRACE_SETREGSET (NT_PRSTATUS/NT_FPREGSET)
- Handles architecture-specific register layouts

### GDB Protocol Definitions
- `Definitions/{ARCH}.json`
- Register set definitions for GDB remote protocol

## Usage Notes

### Active Architectures
Complete ptrace implementations with proper register handling.

### Legacy/Removed Architectures
Implementations provided for:
- Historical debugging of old systems
- Cross-compilation targets
- Academic/research purposes
- Maintaining legacy infrastructure

Note: Legacy architectures removed from mainline Linux may require:
- Older kernel versions
- Custom kernel builds
- Vendor-specific Linux distributions

## Kernel Support Timeline

| Architecture | Added | Removed | Notes |
|--------------|-------|---------|-------|
| S390/S390X | 2.2 | Active | IBM mainframes |
| Hexagon | 3.0 | Active | Qualcomm DSP |
| ARC | 3.9 | Active | Synopsys |
| Xtensa | 2.6 | Active | ESP32, audio DSP |
| OpenRISC | 3.1 | Active | FPGA, open source |
| C-SKY | 4.20 | Active | Chinese embedded |
| MicroBlaze | 2.6.30 | Active | Xilinx FPGA |
| AVR32 | 2.6.19 | 4.12 | Atmel discontinued |
| Blackfin | 2.6.22 | 4.17 | Analog Devices |
| TILE | 2.6.36 | 4.17 | Tilera/Mellanox |
| META | 3.9 | 4.17 | Imagination |
| MN10300 | 2.6.22 | 4.17 | Panasonic |
| M32R | 2.6.13 | 5.1 | Renesas |
| CRIS | 2.0 | 5.4 | Axis Communications |
| SCORE | 2.6.32 | 5.0 | Sunplus |
| FRV | 2.6.7 | 5.7 | Fujitsu |
| Unicore32 | 2.6.39 | 5.9 | Chinese RISC |
| NDS32 | 4.17 | 5.17 | Andes Technology |

## References

- Linux kernel: Documentation/arch/
- GDB: gdb/features/
- QEMU: target/ directory for emulation
- Architecture reference manuals (vendor-specific)
