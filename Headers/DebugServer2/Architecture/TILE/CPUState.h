#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace TILE {
struct CPUState {
  struct { uint64_t r[56]; } gp;  // r0-r55 (TILE-Gx 64-bit)
  uint64_t pc, ex1, faultnum;
};
}}}
