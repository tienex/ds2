#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace META {
struct CPUState {
  struct { uint32_t d[2][8], a[2][4]; } gp;  // Data/Address units
  uint32_t pc, txstatus, txmask;
};
}}}
