#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace Xtensa {
struct CPUState {
  struct { uint32_t a[16]; } ar;  // Address registers
  uint32_t pc, ps, lbeg, lend, lcount, sar;
  struct { uint32_t b[16]; } br;  // Boolean registers
  struct { uint32_t acclo, acchi; } mac16;
};
}}}
