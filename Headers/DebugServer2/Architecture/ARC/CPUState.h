#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace ARC {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31
  uint32_t bta, lp_start, lp_end, lp_count;  // Loop regs
  uint32_t status32, ret, blink;
  uint32_t fp, sp, pc;
};
}}}
