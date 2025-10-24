#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace Blackfin {
struct CPUState {
  struct { uint32_t r[8]; } gp;  // R0-R7
  struct { uint32_t p[6]; } ptr;  // P0-P5
  uint32_t fp, sp, pc, astat, rets, reti, retx, retn, rete;
};
}}}
