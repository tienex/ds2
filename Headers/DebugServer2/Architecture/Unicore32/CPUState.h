#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace Unicore32 {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // r0-r31
  uint32_t pc, asr;  // Application Status Register
};
}}}
