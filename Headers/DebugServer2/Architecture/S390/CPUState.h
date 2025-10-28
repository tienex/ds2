#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace S390 {
struct CPUState {
  struct { uint32_t r[16]; } gp;  // General registers
  struct { uint32_t psw_mask; uint32_t psw_addr; } psw;  // PSW
  struct { uint32_t a[16]; } acr;  // Access registers
  struct { uint64_t f[16]; } fpr;  // FP registers (64-bit)
  uint32_t fpc;  // FP control
};
}}}
