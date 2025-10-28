//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC Book E (Embedded Architecture)
//
// Book E defines the embedded PowerPC architecture used in embedded
// processors (e200, e500, e600 families). Differs significantly from
// Book III-S (server) and Book III-E (desktop) architectures.
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace BookE {

//
// Book E Overview
//
// Book E is the embedded PowerPC architecture specification featuring:
// - Simplified MMU (no segment registers, uses TLBs directly)
// - Enhanced debug facilities (more debug registers)
// - Embedded-specific SPRs
// - Different exception model
// - Optional VLE (Variable Length Encoding) support
//

//
// Book E Special Purpose Registers (SPRs)
//

enum SPRNumbers {
  // Standard PowerPC SPRs (shared with other books)
  SPR_XER         = 1,    // Fixed-point exception register
  SPR_LR          = 8,    // Link register
  SPR_CTR         = 9,    // Count register
  SPR_DEC         = 22,   // Decrementer
  SPR_SRR0        = 26,   // Save/restore register 0
  SPR_SRR1        = 27,   // Save/restore register 1
  SPR_PID         = 48,   // Process ID
  SPR_DECAR       = 54,   // Decrementer auto-reload
  SPR_CSRR0       = 58,   // Critical save/restore register 0
  SPR_CSRR1       = 59,   // Critical save/restore register 1
  SPR_DEAR        = 61,   // Data exception address register
  SPR_ESR         = 62,   // Exception syndrome register
  SPR_IVPR        = 63,   // Interrupt vector prefix register

  // Additional Book E save/restore registers
  SPR_USPRG0      = 256,  // User SPR general 0
  SPR_SPRG0       = 272,  // SPR general 0
  SPR_SPRG1       = 273,  // SPR general 1
  SPR_SPRG2       = 274,  // SPR general 2
  SPR_SPRG3       = 275,  // SPR general 3
  SPR_SPRG4       = 276,  // SPR general 4
  SPR_SPRG5       = 277,  // SPR general 5
  SPR_SPRG6       = 278,  // SPR general 6
  SPR_SPRG7       = 279,  // SPR general 7

  // Time base registers
  SPR_TBL_READ    = 268,  // Time base lower (read)
  SPR_TBU_READ    = 269,  // Time base upper (read)
  SPR_TBL_WRITE   = 284,  // Time base lower (write)
  SPR_TBU_WRITE   = 285,  // Time base upper (write)

  // Interrupt vector offset registers (16 total)
  SPR_IVOR0       = 400,  // Critical input
  SPR_IVOR1       = 401,  // Machine check
  SPR_IVOR2       = 402,  // Data storage
  SPR_IVOR3       = 403,  // Instruction storage
  SPR_IVOR4       = 404,  // External input
  SPR_IVOR5       = 405,  // Alignment
  SPR_IVOR6       = 406,  // Program
  SPR_IVOR7       = 407,  // Floating-point unavailable
  SPR_IVOR8       = 408,  // System call
  SPR_IVOR9       = 409,  // Auxiliary processor unavailable
  SPR_IVOR10      = 410,  // Decrementer
  SPR_IVOR11      = 411,  // Fixed-interval timer interrupt
  SPR_IVOR12      = 412,  // Watchdog timer interrupt
  SPR_IVOR13      = 413,  // Data TLB error
  SPR_IVOR14      = 414,  // Instruction TLB error
  SPR_IVOR15      = 415,  // Debug

  // Debug SPRs
  SPR_DBCR0       = 308,  // Debug control register 0
  SPR_DBCR1       = 309,  // Debug control register 1
  SPR_DBCR2       = 310,  // Debug control register 2
  SPR_IAC1        = 312,  // Instruction address compare 1
  SPR_IAC2        = 313,  // Instruction address compare 2
  SPR_IAC3        = 314,  // Instruction address compare 3
  SPR_IAC4        = 315,  // Instruction address compare 4
  SPR_DAC1        = 316,  // Data address compare 1
  SPR_DAC2        = 317,  // Data address compare 2
  SPR_DVC1        = 318,  // Data value compare 1
  SPR_DVC2        = 319,  // Data value compare 2
  SPR_DBSR        = 304,  // Debug status register
  SPR_DSRR0       = 574,  // Debug save/restore register 0
  SPR_DSRR1       = 575,  // Debug save/restore register 1

  // MMU registers
  SPR_PID0        = 48,   // Process ID register 0
  SPR_PID1        = 633,  // Process ID register 1 (optional)
  SPR_PID2        = 634,  // Process ID register 2 (optional)
  SPR_MMUCFG      = 1015, // MMU configuration
  SPR_TLB0CFG     = 688,  // TLB 0 configuration
  SPR_TLB1CFG     = 689,  // TLB 1 configuration
  SPR_MAS0        = 624,  // MMU assist register 0
  SPR_MAS1        = 625,  // MMU assist register 1
  SPR_MAS2        = 626,  // MMU assist register 2
  SPR_MAS3        = 627,  // MMU assist register 3
  SPR_MAS4        = 628,  // MMU assist register 4
  SPR_MAS5        = 629,  // MMU assist register 5 (optional)
  SPR_MAS6        = 630,  // MMU assist register 6
  SPR_MAS7        = 944,  // MMU assist register 7 (optional)

  // Cache management
  SPR_L1CFG0      = 515,  // L1 cache configuration 0
  SPR_L1CFG1      = 516,  // L1 cache configuration 1
  SPR_L1CSR0      = 1010, // L1 cache control and status 0
  SPR_L1CSR1      = 1011, // L1 cache control and status 1

  // Processor version and identification
  SPR_PVR         = 287,  // Processor version register
  SPR_SVR         = 1023, // System version register (Book E specific)

  // SPE/EFP registers
  SPR_SPEFSCR     = 512,  // SPE floating-point status and control
};

//
// MSR (Machine State Register) bits for Book E
//

enum MSRBits {
  MSR_UCLE   = (1 << 26), // User-mode cache lock enable
  MSR_SPV    = (1 << 25), // SPE available (e500 cores)
  MSR_WE     = (1 << 18), // Wait state enable
  MSR_CE     = (1 << 17), // Critical interrupt enable
  MSR_EE     = (1 << 15), // External interrupt enable
  MSR_PR     = (1 << 14), // Problem state (user mode)
  MSR_FP     = (1 << 13), // Floating-point available
  MSR_ME     = (1 << 12), // Machine check enable
  MSR_FE0    = (1 << 11), // Floating-point exception mode 0
  MSR_DWE    = (1 << 10), // Debug wait enable
  MSR_DE     = (1 <<  9), // Debug interrupt enable
  MSR_FE1    = (1 <<  8), // Floating-point exception mode 1
  MSR_IS     = (1 <<  5), // Instruction address space
  MSR_DS     = (1 <<  4), // Data address space
  MSR_PMM    = (1 <<  2), // Performance monitor mark
};

//
// DBCR0 (Debug Control Register 0) bits
//

enum DBCR0Bits {
  DBCR0_EDM      = (1 << 31), // External debug mode
  DBCR0_IDM      = (1 << 30), // Internal debug mode
  DBCR0_RST_MASK = (3 << 28), // Reset field
  DBCR0_RST_NONE = (0 << 28), // No reset
  DBCR0_RST_CORE = (1 << 28), // Core reset
  DBCR0_RST_CHIP = (2 << 28), // Chip reset
  DBCR0_RST_SYS  = (3 << 28), // System reset
  DBCR0_ICMP     = (1 << 27), // Instruction completion debug event
  DBCR0_BRT      = (1 << 26), // Branch taken debug event
  DBCR0_IRPT     = (1 << 25), // Interrupt taken debug event
  DBCR0_TRAP     = (1 << 24), // Trap instruction debug event
  DBCR0_IAC1     = (1 << 23), // Instruction address compare 1 debug event
  DBCR0_IAC2     = (1 << 22), // Instruction address compare 2 debug event
  DBCR0_IAC3     = (1 << 21), // Instruction address compare 3 debug event
  DBCR0_IAC4     = (1 << 20), // Instruction address compare 4 debug event
  DBCR0_DAC1R    = (1 << 19), // Data address compare 1 read debug event
  DBCR0_DAC1W    = (1 << 18), // Data address compare 1 write debug event
  DBCR0_DAC2R    = (1 << 17), // Data address compare 2 read debug event
  DBCR0_DAC2W    = (1 << 16), // Data address compare 2 write debug event
  DBCR0_RET      = (1 << 15), // Return debug event
  DBCR0_CIRPT    = (1 << 14), // Critical interrupt taken debug event
  DBCR0_CRET     = (1 << 13), // Critical return debug event
  DBCR0_FT       = (1 <<  0), // Freeze timers on debug event
};

//
// DBSR (Debug Status Register) bits
//

enum DBSRBits {
  DBSR_IDE       = (1 << 31), // Imprecise debug event
  DBSR_UDE       = (1 << 30), // Unconditional debug event
  DBSR_MRR_MASK  = (3 << 28), // Most recent reset
  DBSR_ICMP      = (1 << 27), // Instruction completion
  DBSR_BRT       = (1 << 26), // Branch taken
  DBSR_IRPT      = (1 << 25), // Interrupt taken
  DBSR_TRAP      = (1 << 24), // Trap instruction
  DBSR_IAC1      = (1 << 23), // Instruction address compare 1
  DBSR_IAC2      = (1 << 22), // Instruction address compare 2
  DBSR_IAC3      = (1 << 21), // Instruction address compare 3
  DBSR_IAC4      = (1 << 20), // Instruction address compare 4
  DBSR_DAC1R     = (1 << 19), // Data address compare 1 read
  DBSR_DAC1W     = (1 << 18), // Data address compare 1 write
  DBSR_DAC2R     = (1 << 17), // Data address compare 2 read
  DBSR_DAC2W     = (1 << 16), // Data address compare 2 write
  DBSR_RET       = (1 << 15), // Return
  DBSR_CIRPT     = (1 << 14), // Critical interrupt taken
  DBSR_CRET      = (1 << 13), // Critical return
};

//
// ESR (Exception Syndrome Register) bits
//

enum ESRBits {
  ESR_PIL        = (1 << 27), // Program interrupt - illegal instruction
  ESR_PPR        = (1 << 26), // Program interrupt - privileged instruction
  ESR_PTR        = (1 << 25), // Program interrupt - trap
  ESR_FP         = (1 << 24), // Floating-point operation
  ESR_ST         = (1 << 23), // Store operation
  ESR_DLK        = (1 << 21), // Data cache locking
  ESR_ILK        = (1 << 20), // Instruction cache locking
  ESR_AP         = (1 << 19), // Auxiliary processor operation
  ESR_PUO        = (1 << 18), // Unimplemented operation
  ESR_BO         = (1 << 17), // Byte ordering
  ESR_PIE        = (1 << 16), // Program interrupt - exception
  ESR_SPV        = (1 << 15), // SPE/embedded FP/AltiVec operation
  ESR_VLEMI      = (1 << 11), // VLE mode instruction
  ESR_MIF        = (1 << 10), // Misaligned instruction fetch
  ESR_XTE        = (1 <<  0), // External transaction error
};

//
// Book E Debug State
//

struct DebugState {
  uint32_t dbcr0;         // Debug control register 0
  uint32_t dbcr1;         // Debug control register 1
  uint32_t dbcr2;         // Debug control register 2
  uint32_t iac1;          // Instruction address compare 1
  uint32_t iac2;          // Instruction address compare 2
  uint32_t iac3;          // Instruction address compare 3
  uint32_t iac4;          // Instruction address compare 4
  uint32_t dac1;          // Data address compare 1
  uint32_t dac2;          // Data address compare 2
  uint32_t dvc1;          // Data value compare 1
  uint32_t dvc2;          // Data value compare 2
  uint32_t dbsr;          // Debug status register
  uint32_t dsrr0;         // Debug save/restore register 0
  uint32_t dsrr1;         // Debug save/restore register 1
};

//
// Book E TLB Entry Format (simplified)
//

struct TLBEntry {
  // MAS0 fields
  uint32_t tlbsel  : 2;   // TLB select
  uint32_t esel    : 12;  // Entry select

  // MAS1 fields
  uint32_t valid   : 1;   // Entry valid
  uint32_t iprot   : 1;   // Invalidate protect
  uint32_t tid     : 8;   // Translation ID (PID)
  uint32_t ts      : 1;   // Translation space
  uint32_t tsize   : 4;   // Page size (2^(TSIZE+10) bytes)

  // MAS2 fields
  uint32_t epn;           // Effective page number
  uint32_t wimge   : 5;   // Write-through, Caching inhibited, Memory coherency, Guarded, Endianness

  // MAS3/MAS7 fields
  uint64_t rpn;           // Real page number (36-40 bits depending on processor)
  uint32_t u0      : 1;   // User attribute 0
  uint32_t u1      : 1;   // User attribute 1
  uint32_t u2      : 1;   // User attribute 2
  uint32_t u3      : 1;   // User attribute 3
  uint32_t ux      : 1;   // User execute permission
  uint32_t sx      : 1;   // Supervisor execute permission
  uint32_t uw      : 1;   // User write permission
  uint32_t sw      : 1;   // Supervisor write permission
  uint32_t ur      : 1;   // User read permission
  uint32_t sr      : 1;   // Supervisor read permission
};

//
// Book E Exception Vectors (offsets from IVPR)
//

enum ExceptionVectors {
  IVOR_CRITICAL_INPUT     = 0,   // Critical input
  IVOR_MACHINE_CHECK      = 1,   // Machine check
  IVOR_DATA_STORAGE       = 2,   // Data storage
  IVOR_INST_STORAGE       = 3,   // Instruction storage
  IVOR_EXTERNAL_INPUT     = 4,   // External input
  IVOR_ALIGNMENT          = 5,   // Alignment
  IVOR_PROGRAM            = 6,   // Program
  IVOR_FP_UNAVAILABLE     = 7,   // Floating-point unavailable
  IVOR_SYSTEM_CALL        = 8,   // System call
  IVOR_AUX_UNAVAILABLE    = 9,   // Auxiliary processor unavailable
  IVOR_DECREMENTER        = 10,  // Decrementer
  IVOR_FIT                = 11,  // Fixed-interval timer
  IVOR_WATCHDOG           = 12,  // Watchdog timer
  IVOR_DATA_TLB_ERROR     = 13,  // Data TLB error
  IVOR_INST_TLB_ERROR     = 14,  // Instruction TLB error
  IVOR_DEBUG              = 15,  // Debug
};

//
// Book E CPU State Extensions
//

struct BookEState {
  // Standard Book E SPRs
  uint32_t pid;           // Process ID
  uint32_t decar;         // Decrementer auto-reload
  uint32_t csrr0;         // Critical save/restore 0
  uint32_t csrr1;         // Critical save/restore 1
  uint32_t dear;          // Data exception address
  uint32_t esr;           // Exception syndrome
  uint32_t ivpr;          // Interrupt vector prefix

  // SPRG registers (8 total)
  uint32_t sprg[8];

  // IVOR registers (16 total)
  uint32_t ivor[16];

  // Debug state
  DebugState debug;

  // MMU assist registers
  uint32_t mas[8];        // MAS0-MAS7

  // Cache control
  uint32_t l1csr0;        // L1 cache control 0 (instruction)
  uint32_t l1csr1;        // L1 cache control 1 (data)
};

} // namespace BookE
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
