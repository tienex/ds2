#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace Elbrus2K {
struct CPUState {
  struct { uint64_t r[32]; } gp;  // General regs (based/global/local/stack)
  struct { uint64_t p[32]; } pred;  // Predicate registers
  uint64_t ip, nip;  // IP, next IP
  struct { uint32_t psr, upsr; } status;
};
}}}
