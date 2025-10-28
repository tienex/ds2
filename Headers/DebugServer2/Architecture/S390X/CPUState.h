#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace S390X {
struct CPUState {
  struct { uint64_t r[16]; } gp;  // General registers
  struct { uint64_t psw_mask; uint64_t psw_addr; } psw;  // PSW
  struct { uint32_t a[16]; } acr;  // Access registers
  struct { uint64_t f[16]; } fpr;  // FP registers
  uint32_t fpc;  // FP control
  struct { uint64_t vr[32][2]; } vr;  // Vector registers (z13+)
};
}}}
