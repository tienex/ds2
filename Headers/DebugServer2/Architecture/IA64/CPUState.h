//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Architecture/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace IA64 {

//
// IA-64 (Itanium) CPU State
// Covers: Itanium, Itanium 2, Itanium 9300/9500/9700 series (Merced through Kittson)
//

// Floating-point register - 82 bits (64-bit significand + 17-bit exponent + sign)
struct FPRegister {
  uint64_t significand;  // 64-bit significand
  uint32_t exponent : 17; // 17-bit exponent
  uint32_t sign : 1;      // Sign bit
  uint32_t _reserved : 14;
};

struct CPUState {
  //
  // General Registers (GR0-GR127)
  // 128 general-purpose 64-bit registers
  // GR0 is always zero (hardwired)
  //
  struct {
    union {
      uint64_t regs[128];
      struct {
        uint64_t gr0, gr1, gr2, gr3, gr4, gr5, gr6, gr7;
        uint64_t gr8, gr9, gr10, gr11, gr12, gr13, gr14, gr15;
        uint64_t gr16, gr17, gr18, gr19, gr20, gr21, gr22, gr23;
        uint64_t gr24, gr25, gr26, gr27, gr28, gr29, gr30, gr31;
        uint64_t gr32, gr33, gr34, gr35, gr36, gr37, gr38, gr39;
        uint64_t gr40, gr41, gr42, gr43, gr44, gr45, gr46, gr47;
        uint64_t gr48, gr49, gr50, gr51, gr52, gr53, gr54, gr55;
        uint64_t gr56, gr57, gr58, gr59, gr60, gr61, gr62, gr63;
        uint64_t gr64, gr65, gr66, gr67, gr68, gr69, gr70, gr71;
        uint64_t gr72, gr73, gr74, gr75, gr76, gr77, gr78, gr79;
        uint64_t gr80, gr81, gr82, gr83, gr84, gr85, gr86, gr87;
        uint64_t gr88, gr89, gr90, gr91, gr92, gr93, gr94, gr95;
        uint64_t gr96, gr97, gr98, gr99, gr100, gr101, gr102, gr103;
        uint64_t gr104, gr105, gr106, gr107, gr108, gr109, gr110, gr111;
        uint64_t gr112, gr113, gr114, gr115, gr116, gr117, gr118, gr119;
        uint64_t gr120, gr121, gr122, gr123, gr124, gr125, gr126, gr127;
      };
    };
  } gp;

  //
  // Floating-Point Registers (FR0-FR127)
  // 128 floating-point registers, 82 bits each
  // FR0 = +0.0, FR1 = +1.0 (hardwired)
  //
  FPRegister fpr[128];

  //
  // Predicate Registers (PR0-PR63)
  // 64 predicate registers, 1 bit each
  // Stored as 64-bit value with one bit per predicate
  // PR0 is always 1 (hardwired)
  //
  uint64_t pr;

  //
  // Branch Registers (BR0-BR7)
  // 8 branch target registers
  //
  uint64_t br[8];

  //
  // Application Registers
  //
  struct {
    // Kernel registers (AR0-AR7)
    uint64_t kr[8];          // AR0-AR7: Kernel registers

    // Register stack engine
    uint64_t rsc;            // AR16: Register Stack Configuration
    uint64_t bsp;            // AR17: Backing Store Pointer
    uint64_t bspstore;       // AR18: BSP Store
    uint64_t rnat;           // AR19: RSE NaT Collection

    // Reserved AR20
    uint64_t _reserved_ar20;

    // Function call registers
    uint64_t fcr;            // AR21: Function Call Register

    // Reserved AR22-AR23
    uint64_t _reserved_ar22_23[2];

    // EFLAG (x86 compatibility)
    uint64_t eflag;          // AR24: EFLAG (for IA-32 emulation)

    // Segmentation (x86 compatibility)
    uint64_t csd;            // AR25: Code Segment Descriptor
    uint64_t ssd;            // AR26: Stack Segment Descriptor
    uint64_t cflg;           // AR27: Compatibility FLAGS

    // User/system registers
    uint64_t fsr;            // AR28: Floating-point Status Register
    uint64_t fir;            // AR29: Floating-point Instruction Register
    uint64_t fdr;            // AR30: Floating-point Data Register

    // Reserved AR31
    uint64_t _reserved_ar31;

    // Compare and exchange
    uint64_t ccv;            // AR32: Compare and Exchange Compare Value

    // Reserved AR33-AR35
    uint64_t _reserved_ar33_35[3];

    // User NaT collection
    uint64_t unat;           // AR36: User NaT Collection

    // Reserved AR37-AR39
    uint64_t _reserved_ar37_39[3];

    // Floating-point status
    uint64_t fpsr;           // AR40: Floating-Point Status Register

    // Reserved AR41-AR43
    uint64_t _reserved_ar41_43[3];

    // Interval timer
    uint64_t itc;            // AR44: Interval Time Counter

    // Reserved AR45-AR47
    uint64_t _reserved_ar45_47[3];

    // Reserved AR48-AR63
    uint64_t _reserved_ar48_63[16];

    // Performance monitoring (AR64-AR127)
    uint64_t pmc[8];         // AR64-AR71: Performance Monitor Configuration
    uint64_t pmd[8];         // AR72-AR79: Performance Monitor Data

    // Reserved AR80-AR127
    uint64_t _reserved_ar80_127[48];
  } ar;

  //
  // Control Registers
  //
  struct {
    uint64_t dcr;            // Default Control Register
    uint64_t itm;            // Interval Time Match
    uint64_t iva;            // Interruption Vector Address
    uint64_t pta;            // Page Table Address
    uint64_t ipsr;           // Interruption PSR
    uint64_t isr;            // Interruption Status Register
    uint64_t iip;            // Interruption IP
    uint64_t ifa;            // Interruption Faulting Address
    uint64_t itir;           // Interruption TLB Insertion Register
    uint64_t iipa;           // Interruption Previous IP
    uint64_t ifs;            // Interruption Function State
    uint64_t iim;            // Interruption Immediate
    uint64_t iha;            // Interruption Hash Address

    // Reserved CR13-CR15
    uint64_t _reserved_cr13_15[3];

    // Processor status
    uint64_t lid;            // Local ID
    uint64_t ivr;            // Interruption Vector Register
    uint64_t tpr;            // Task Priority Register
    uint64_t eoi;            // End Of Interrupt
    uint64_t irr[4];         // Interruption Request Register
    uint64_t itv;            // Interval Timer Vector
    uint64_t pmv;            // Performance Monitor Vector
    uint64_t cmcv;           // Corrected Machine Check Vector

    // Reserved CR28-CR63
    uint64_t _reserved_cr28_63[36];

    // Local interrupt control
    uint64_t lrr[2];         // Local Redirection Register 0-1

    // Reserved CR66-CR79
    uint64_t _reserved_cr66_79[14];
  } cr;

  //
  // Processor Status Register (PSR)
  //
  struct {
    uint64_t rv : 1;         // Reserved (bit 0)
    uint64_t be : 1;         // Big-Endian
    uint64_t up : 1;         // User Performance monitor enable
    uint64_t ac : 1;         // Alignment Check
    uint64_t mfl : 1;        // Lower floating-point registers written
    uint64_t mfh : 1;        // Upper floating-point registers written
    uint64_t _reserved1 : 7; // Reserved bits 6-12
    uint64_t ic : 1;         // Interruption Collection
    uint64_t i : 1;          // Interrupt enable
    uint64_t pk : 1;         // Protection Key enable
    uint64_t _reserved2 : 1; // Reserved bit 16
    uint64_t dt : 1;         // Data Address Translation
    uint64_t dfl : 1;        // Disabled Floating-point Low register set
    uint64_t dfh : 1;        // Disabled Floating-point High register set
    uint64_t sp : 1;         // Secure Performance monitors
    uint64_t pp : 1;         // Privileged Performance monitor enable
    uint64_t di : 1;         // Disable Instruction set transition
    uint64_t si : 1;         // Secure Interval timer
    uint64_t db : 1;         // Debug Breakpoint fault
    uint64_t lp : 1;         // Lower Privilege transfer trap
    uint64_t tb : 1;         // Taken Branch trap
    uint64_t rt : 1;         // Register stack translation
    uint64_t _reserved3 : 4; // Reserved bits 28-31
    uint64_t cpl : 2;        // Current Privilege Level
    uint64_t is : 1;         // Instruction Set (0=IA-64, 1=IA-32)
    uint64_t mc : 1;         // Machine Check abort mask
    uint64_t it : 1;         // Instruction address Translation
    uint64_t id : 1;         // Instruction Debug fault disable
    uint64_t da : 1;         // Disable Data access and dirty-bit faults
    uint64_t dd : 1;         // Data Debug fault disable
    uint64_t ss : 1;         // Single Step enable
    uint64_t ri : 2;         // Restart Instruction (slot number)
    uint64_t ed : 1;         // Exception Deferral
    uint64_t bn : 1;         // Register Bank
    uint64_t ia : 1;         // Disable Instruction Access-bit faults
    uint64_t _reserved4 : 20;// Reserved bits 45-63
  } psr;

  //
  // Current Frame Marker (CFM)
  //
  struct {
    uint64_t sof : 7;        // Size of Frame
    uint64_t sol : 7;        // Size of Locals
    uint64_t sor : 4;        // Size of Rotating portion
    uint64_t rrb_gr : 7;     // Rotating Register Base for GRs
    uint64_t rrb_fr : 7;     // Rotating Register Base for FRs
    uint64_t rrb_pr : 6;     // Rotating Register Base for PRs
    uint64_t _reserved : 26; // Reserved
  } cfm;

  //
  // Instruction Pointer (IP)
  //
  uint64_t ip;

  //
  // Region Registers (RR0-RR7)
  // Control virtual address space regions
  //
  struct {
    uint64_t ve : 1;         // VHPT Enable
    uint64_t _reserved1 : 1; // Reserved
    uint64_t ps : 6;         // Preferred Page Size
    uint64_t rid : 24;       // Region ID
    uint64_t _reserved2 : 32;// Reserved
  } rr[8];

  //
  // Protection Key Registers (PKR0-PKR15)
  // Memory access rights
  //
  struct {
    uint64_t v : 1;          // Valid
    uint64_t wr : 1;         // Write Disable
    uint64_t rd : 1;         // Read Disable
    uint64_t xd : 1;         // Execute Disable
    uint64_t _reserved1 : 4; // Reserved
    uint64_t key : 24;       // Protection Key
    uint64_t _reserved2 : 32;// Reserved
  } pkr[16];

  //
  // Debug Registers
  //
  struct {
    // Instruction breakpoints (IBR0-IBR7 pairs)
    struct {
      uint64_t addr;         // Instruction address
      uint64_t mask;         // Address mask
    } ibr[8];

    // Data breakpoints (DBR0-DBR7 pairs)
    struct {
      uint64_t addr;         // Data address
      uint64_t mask;         // Address mask and controls
    } dbr[8];
  } debug;

  //
  // NaT (Not a Thing) bits
  // Correspond to general registers
  // Stored as 128-bit value (one bit per GR)
  //
  struct {
    uint64_t nat_low;        // NaT bits for GR0-GR63
    uint64_t nat_high;       // NaT bits for GR64-GR127
  } nat;

  //
  // Performance Monitoring (Itanium 9300+ features)
  //
  struct {
    uint64_t pmc[256];       // Performance Monitor Configuration (extended)
    uint64_t pmd[256];       // Performance Monitor Data (extended)
    uint64_t opcode_matcher[4]; // Opcode matchers (Montecito+)
    uint64_t iaddr_matcher[4];  // Instruction address matchers
    uint64_t daddr_matcher[4];  // Data address matchers
  } perfmon;

  //
  // Virtual Hash Page Table (VHPT)
  //
  struct {
    uint64_t pta;            // Page Table Address register
    uint64_t impl_va_msb : 8;// Implementation VA MSB
    uint64_t _reserved1 : 7; // Reserved
    uint64_t ve : 1;         // VHPT Enable
    uint64_t _reserved2 : 6; // Reserved
    uint64_t size : 6;       // VHPT size
    uint64_t vf : 1;         // VHPT Format (0=short, 1=long)
    uint64_t base : 35;      // VHPT base address
  } vhpt;

  //
  // Machine Check / Error Handling
  //
  struct {
    uint64_t mca[128];       // Machine Check Abort registers
    uint64_t pal_mc_clear;   // PAL Machine Check Clear
    uint64_t pal_mc_resume;  // PAL Machine Check Resume
  } mca;

  //
  // CPUID Information
  //
  struct {
    uint64_t cpuid[5];       // CPUID registers 0-4
  } cpuid;

  //
  // Advanced features (Itanium 2 9000 series+)
  //
  struct {
    // Dual-Core / Multi-core support (Montecito+)
    uint64_t core_id;        // Physical core ID
    uint64_t thread_id;      // Thread ID (for HT)

    // Virtualization (Tukwila+)
    uint64_t virt_enable;    // Virtualization enabled
    uint64_t vmx_controls;   // VMX control registers

    // Power management (Poulson+)
    uint64_t power_state;    // Current power state
    uint64_t freq_ratio;     // Frequency ratio

    // RAS features (Kittson)
    uint64_t ras_caps;       // RAS capabilities
    uint64_t error_log[8];   // Extended error logging
  } advanced;
};

} // namespace IA64
} // namespace Architecture
} // namespace ds2
