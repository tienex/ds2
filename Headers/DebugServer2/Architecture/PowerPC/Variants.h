//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC Architecture Variants
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {

//
// PowerPC / POWER Processor Variants
//
// This file documents the evolution of PowerPC/POWER architecture
// from PowerPC 601 (1993) through POWER10 (2021)
//

enum class Variant {
  // Classic 32-bit PowerPC (1993-2006)
  PPC_601,       // PowerPC 601 - First PowerPC chip (hybrid POWER/PowerPC)
  PPC_603,       // PowerPC 603 - Low-power mobile
  PPC_603e,      // PowerPC 603e - Enhanced 603
  PPC_604,       // PowerPC 604 - Desktop/workstation
  PPC_604e,      // PowerPC 604e - Enhanced 604
  PPC_750,       // PowerPC 750 (G3) - Apple's G3, better branch prediction
  PPC_7400,      // PowerPC 7400 (G4) - First with AltiVec/VMX
  PPC_7450,      // PowerPC 7450 (G4+) - Enhanced G4 with better AltiVec
  PPC_7455,      // PowerPC 7455 (G4e) - Dual-core capable

  // 64-bit PowerPC (2003-2006)
  PPC_970,       // PowerPC 970 (G5) - Desktop 64-bit, derived from POWER4
  PPC_970FX,     // PowerPC 970FX - Lower power G5
  PPC_970MP,     // PowerPC 970MP - Dual-core G5

  // Embedded PowerPC (Book E architecture)
  PPC_E200,      // e200 - Low-end embedded with VLE
  PPC_E500,      // e500 - Freescale embedded (SPE instead of AltiVec)
  PPC_E500MC,    // e500mc - Multi-core e500 without SPE
  PPC_E500V2,    // e500v2 - Enhanced e500 with double-precision SPE
  PPC_E5500,     // e5500 - 64-bit embedded
  PPC_E6500,     // e6500 - High-performance embedded with AltiVec
  PPC_E7500,     // e7500 - High-performance multi-core embedded
  QORIVVA,       // Qorivva - NXP automotive MCU (VLE capable)

  // Game console PowerPC
  GEKKO,         // Gekko - Nintendo GameCube (750-based)
  BROADWAY,      // Broadway - Nintendo Wii (750-based)
  ESPRESSO,      // Espresso - Nintendo Wii U (750-based, tri-core)
  XENON,         // Xenon - Xbox 360 (custom tri-core with VMX128)

  // Cell Broadband Engine (2006)
  CELL_PPU,      // Cell PPE (PowerPC Processing Element) - PS3, IBM blades
  CELL_SPU,      // Cell SPE (Synergistic Processing Element) - unique ISA

  // IBM POWER Server (64-bit)
  POWER4,        // POWER4 (2001) - First 64-bit POWER, SMT
  POWER4_PLUS,   // POWER4+ - Enhanced POWER4
  POWER5,        // POWER5 (2004) - Improved SMT, virtualization
  POWER5_PLUS,   // POWER5+ - Enhanced POWER5
  POWER6,        // POWER6 (2007) - High frequency (4.7 GHz+)
  POWER7,        // POWER7 (2010) - VSX (Vector-Scalar Extension)
  POWER7_PLUS,   // POWER7+ - Enhanced POWER7, TurboCore
  POWER8,        // POWER8 (2013) - Transactional memory, CAPI
  POWER9,        // POWER9 (2017) - NVLink, OpenCAPI, Meltdown-immune
  POWER10,       // POWER10 (2021) - PCIe Gen 5, DDR5, Matrix Math Accelerator

  // IBM AS/400 PowerPC-AS (IMPI processors with TIMI)
  AMAZON,        // Amazon - First 64-bit AS/400 PowerPC-AS (IMPI)
  APACHE,        // Apache - Enhanced Amazon for AS/400
  NORTHSTAR,     // Northstar - Advanced AS/400 processor
  PULSAR,        // Pulsar - High-performance AS/400 processor
  ISTAR,         // iStar - iSeries IMPI processor
  SSTAR,         // SStar - System i processor
};

// Processor capabilities/features
struct CapabilityFlags {
  bool has_64bit;        // 64-bit addressing (PPC64)
  bool has_altivec;      // AltiVec/VMX SIMD (G4+)
  bool has_vsx;          // Vector-Scalar Extension (POWER7+)
  bool has_spe;          // Signal Processing Engine (e500)
  bool has_dfp;          // Decimal Floating Point (POWER6+)
  bool has_htm;          // Hardware Transactional Memory (POWER8+)
  bool has_smt;          // Simultaneous Multithreading (POWER4+)
  bool has_vmx128;       // VMX128 extension (Xbox 360)
  bool has_mma;          // Matrix Math Accelerator (POWER10)
  bool has_vle;          // Variable Length Encoding (e200, Qorivva)
  bool has_booke;        // Book E embedded architecture
  bool has_impi;         // IMPI (AS/400 processors)
  bool has_timi;         // TIMI MI translation (AS/400)
};

// Get capabilities for a specific variant
inline CapabilityFlags GetCapabilities(Variant variant) {
  CapabilityFlags caps = {};

  switch (variant) {
    // Classic 32-bit without AltiVec
    case Variant::PPC_601:
    case Variant::PPC_603:
    case Variant::PPC_603e:
    case Variant::PPC_604:
    case Variant::PPC_604e:
    case Variant::PPC_750:
    case Variant::GEKKO:
    case Variant::BROADWAY:
      caps.has_64bit = false;
      caps.has_altivec = false;
      break;

    // 32-bit with AltiVec
    case Variant::PPC_7400:
    case Variant::PPC_7450:
    case Variant::PPC_7455:
      caps.has_64bit = false;
      caps.has_altivec = true;
      break;

    // 64-bit desktop PowerPC
    case Variant::PPC_970:
    case Variant::PPC_970FX:
    case Variant::PPC_970MP:
      caps.has_64bit = true;
      caps.has_altivec = true;
      break;

    // Embedded with VLE (Book E)
    case Variant::PPC_E200:
      caps.has_64bit = false;
      caps.has_vle = true;
      caps.has_booke = true;
      break;

    case Variant::QORIVVA:
      caps.has_64bit = false;
      caps.has_vle = true;
      caps.has_booke = true;
      caps.has_spe = true;
      break;

    // Embedded with SPE (Book E)
    case Variant::PPC_E500:
    case Variant::PPC_E500V2:
      caps.has_64bit = false;
      caps.has_spe = true;
      caps.has_booke = true;
      break;

    case Variant::PPC_E500MC:
      caps.has_64bit = false;
      caps.has_booke = true;
      break;

    case Variant::PPC_E5500:
    case Variant::PPC_E6500:
    case Variant::PPC_E7500:
      caps.has_64bit = true;
      caps.has_spe = (variant != Variant::PPC_E500MC);
      caps.has_altivec = (variant == Variant::PPC_E6500 || variant == Variant::PPC_E7500);
      caps.has_booke = true;
      break;

    // Game consoles
    case Variant::ESPRESSO:
      caps.has_64bit = false;
      caps.has_altivec = true;
      break;

    case Variant::XENON:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_vmx128 = true;  // Custom VMX128 extension
      break;

    // Cell
    case Variant::CELL_PPU:
      caps.has_64bit = true;
      caps.has_altivec = true;
      break;

    case Variant::CELL_SPU:
      // SPU is a different architecture entirely
      caps.has_64bit = false;
      break;

    // POWER4-POWER6
    case Variant::POWER4:
    case Variant::POWER4_PLUS:
    case Variant::POWER5:
    case Variant::POWER5_PLUS:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_smt = true;
      break;

    case Variant::POWER6:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_smt = true;
      caps.has_dfp = true;  // Decimal floating point
      break;

    // POWER7+ with VSX
    case Variant::POWER7:
    case Variant::POWER7_PLUS:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_vsx = true;
      caps.has_smt = true;
      caps.has_dfp = true;
      break;

    // POWER8+ with transactional memory
    case Variant::POWER8:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_vsx = true;
      caps.has_smt = true;
      caps.has_dfp = true;
      caps.has_htm = true;
      break;

    // POWER9
    case Variant::POWER9:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_vsx = true;
      caps.has_smt = true;
      caps.has_dfp = true;
      caps.has_htm = true;
      break;

    // POWER10
    case Variant::POWER10:
      caps.has_64bit = true;
      caps.has_altivec = true;
      caps.has_vsx = true;
      caps.has_smt = true;
      caps.has_dfp = true;
      caps.has_htm = true;
      caps.has_mma = true;  // Matrix Math Accelerator
      break;

    // AS/400 PowerPC-AS (IMPI with TIMI)
    case Variant::AMAZON:
    case Variant::APACHE:
    case Variant::NORTHSTAR:
    case Variant::PULSAR:
    case Variant::ISTAR:
    case Variant::SSTAR:
      caps.has_64bit = true;
      caps.has_altivec = false;  // AS/400 processors don't use AltiVec
      caps.has_impi = true;      // IMPI architecture
      caps.has_timi = true;      // TIMI MI translation layer
      caps.has_smt = true;       // Support multithreading
      break;
  }

  return caps;
}

// Processor Version Register (PVR) values for identification
namespace PVR {
  constexpr uint32_t PPC_601       = 0x00010000;
  constexpr uint32_t PPC_603       = 0x00030000;
  constexpr uint32_t PPC_603e      = 0x00060000;
  constexpr uint32_t PPC_604       = 0x00040000;
  constexpr uint32_t PPC_604e      = 0x00090000;
  constexpr uint32_t PPC_750       = 0x00080000;  // G3
  constexpr uint32_t PPC_7400      = 0x000C0000;  // G4
  constexpr uint32_t PPC_7450      = 0x80000000;  // G4+
  constexpr uint32_t PPC_970       = 0x00390000;  // G5
  constexpr uint32_t PPC_970FX     = 0x003C0000;
  constexpr uint32_t PPC_970MP     = 0x00440000;

  constexpr uint32_t GEKKO         = 0x00083214;  // GameCube
  constexpr uint32_t BROADWAY      = 0x00087102;  // Wii
  constexpr uint32_t ESPRESSO      = 0x00087000;  // Wii U

  constexpr uint32_t CELL_PPU      = 0x00700100;  // PS3 / Cell PPE

  // Embedded PowerPC (Book E)
  constexpr uint32_t PPC_E200      = 0x81000000;  // e200 core
  constexpr uint32_t PPC_E500      = 0x80200000;  // e500 core
  constexpr uint32_t PPC_E500V2    = 0x80210000;  // e500v2 core
  constexpr uint32_t PPC_E500MC    = 0x80230000;  // e500mc core
  constexpr uint32_t PPC_E5500     = 0x80240000;  // e5500 core
  constexpr uint32_t PPC_E6500     = 0x80400000;  // e6500 core
  constexpr uint32_t QORIVVA       = 0x81100000;  // Qorivva (e200-based)

  // IBM POWER Server
  constexpr uint32_t POWER4        = 0x00350000;
  constexpr uint32_t POWER4_PLUS   = 0x00380000;
  constexpr uint32_t POWER5        = 0x003A0000;
  constexpr uint32_t POWER5_PLUS   = 0x003B0000;
  constexpr uint32_t POWER6        = 0x003E0000;
  constexpr uint32_t POWER7        = 0x003F0000;
  constexpr uint32_t POWER8E       = 0x004B0000;
  constexpr uint32_t POWER8        = 0x004D0000;
  constexpr uint32_t POWER9        = 0x004E0000;
  constexpr uint32_t POWER10       = 0x00800000;

  // AS/400 PowerPC-AS (IMPI)
  constexpr uint32_t AMAZON        = 0x00410000;  // Amazon
  constexpr uint32_t APACHE        = 0x00420000;  // Apache
  constexpr uint32_t NORTHSTAR     = 0x00430000;  // Northstar
  constexpr uint32_t PULSAR        = 0x00440000;  // Pulsar
  constexpr uint32_t ISTAR         = 0x00450000;  // iStar
  constexpr uint32_t SSTAR         = 0x00460000;  // SStar
}

} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
