#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace C6X {
struct CPUState {
  struct { uint32_t a[16], b[16]; } gp;  // A0-A15, B0-B15
  uint32_t pc, csr, ier, ilc, rilc;
};
}}}
