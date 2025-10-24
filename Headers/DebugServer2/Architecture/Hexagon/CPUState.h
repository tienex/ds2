#pragma once
#include "DebugServer2/Architecture/CPUState.h"
namespace ds2 { namespace Architecture { namespace Hexagon {
struct CPUState {
  struct { uint32_t r[32]; } gp;  // R0-R31
  struct { uint32_t sa0, lc0, sa1, lc1; } hw_loop;  // Hardware loops
  struct { uint64_t p[4]; } pred;  // Predicate registers
  uint32_t pc, usr, gp_reg, ugp;
  struct { uint32_t m0, m1; } modifier;
};
}}}
