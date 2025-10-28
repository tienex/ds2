#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace H8300 {
struct CPUState {
  struct { uint32_t er[8]; } gp;  // ER0-ER7 (32-bit extended)
  uint32_t pc; uint16_t ccr, exr;
};
}}}
