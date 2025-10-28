#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace CSKY {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31 (r14=sp, r15=lr)
  uint32_t pc, psr;
  struct { uint32_t hi, lo; } dsp;
  struct { uint64_t vr[32]; } fpu;  // FPU/Vector
};
}}}
