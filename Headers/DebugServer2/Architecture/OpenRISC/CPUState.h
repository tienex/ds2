#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace OpenRISC {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31 (r0=0, r1=sp, r2=fp, r9=lr)
  uint32_t pc, sr;  // PC, Supervision Register
  uint32_t epcr, eear, esr;  // Exception regs
};
}}}
