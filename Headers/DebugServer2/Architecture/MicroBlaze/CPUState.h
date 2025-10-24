#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace MicroBlaze {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31
  uint32_t pc, msr, ear, esr, fsr;
  struct { uint32_t tlblo[4], tlbhi[4]; } tlb;
};
}}}
