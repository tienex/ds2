#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace MN10300 {
struct CPUState {
  struct { uint32_t d[4], a[4]; } gp;  // Data/Address
  uint32_t pc, psw, mdr, mcvf, mcrl, mcrh;
};
}}}
