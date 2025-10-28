#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace CRIS {
struct CPUState {
  struct { uint32_t r[16]; } gp;  // r0-r15
  uint32_t pc, ccs, spc, mof, srs, erp;
};
}}}
