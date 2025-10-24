#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace NDS32 {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31
  uint32_t pc, d0lo, d0hi, d1lo, d1hi;  // MAC regs
};
}}}
