#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace M32R {
struct CPUState {
  struct { uint32_t r[16]; } gp;  // r0-r15
  uint32_t pc, psw, cbr, spi, spu, bpc;
};
}}}
