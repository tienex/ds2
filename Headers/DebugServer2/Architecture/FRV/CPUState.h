#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace FRV {
struct CPUState {
  struct { uint32_t gr[64]; } gp;  // General regs
  uint32_t pc, psr, ccr, cccr;
};
}}}
