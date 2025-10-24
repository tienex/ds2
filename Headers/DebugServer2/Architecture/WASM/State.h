//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// WebAssembly (WASM) Virtual Architecture
//
// WASM is a portable virtual ISA for web and standalone execution
//

#pragma once

#include <cstdint>
#include <vector>

namespace ds2 {
namespace Architecture {
namespace WASM {

//
// WebAssembly Overview
//
// WebAssembly is a portable, stack-based virtual ISA:
// - Binary instruction format for stack-based virtual machine
// - Designed for fast decoding and execution
// - Used in web browsers and standalone runtimes
// - Supports multiple value types: i32, i64, f32, f64, v128
// - Linear memory model
// - Structured control flow
// - DWARF debug info support for source-level debugging
//

//
// WASM Value Types
//

enum class ValueType : uint8_t {
  I32 = 0x7F,       // 32-bit integer
  I64 = 0x7E,       // 64-bit integer
  F32 = 0x7D,       // 32-bit float
  F64 = 0x7C,       // 64-bit double
  V128 = 0x7B,      // 128-bit SIMD vector
  FUNCREF = 0x70,   // Function reference
  EXTERNREF = 0x6F, // External reference (host object)
};

//
// WASM Value
//

union Value {
  int32_t i32;
  int64_t i64;
  float f32;
  double f64;
  uint8_t v128[16];  // 128-bit SIMD vector
  void *ref;         // Function or external reference
};

struct TypedValue {
  ValueType type;
  Value value;
};

//
// WASM Execution State
//

struct ExecutionState {
  // Program counter (instruction offset in current function)
  uint32_t pc;

  // Current function index
  uint32_t function_index;

  // Value stack (operand stack)
  std::vector<TypedValue> value_stack;

  // Local variables (includes function parameters)
  std::vector<TypedValue> locals;

  // Global variables
  std::vector<TypedValue> globals;

  // Call stack (frame pointers)
  struct Frame {
    uint32_t function_index;
    uint32_t return_pc;
    uint32_t locals_base;    // Base index in locals array
    uint32_t stack_base;     // Base index in value stack
  };
  std::vector<Frame> call_stack;

  inline void clear() {
    pc = 0;
    function_index = 0;
    value_stack.clear();
    locals.clear();
    globals.clear();
    call_stack.clear();
  }
};

//
// WASM Memory Instance
//

struct MemoryInstance {
  uint8_t *data;            // Linear memory data
  uint32_t current_pages;   // Current size in pages (64KB each)
  uint32_t max_pages;       // Maximum size in pages (optional limit)
  bool is_shared;           // Shared memory (for threading)

  inline uint32_t size_bytes() const { return current_pages * 65536; }
};

//
// WASM Table Instance (for indirect calls)
//

struct TableInstance {
  std::vector<void *> elements;  // Function references or external refs
  uint32_t max_size;             // Maximum table size
  ValueType element_type;        // Element type (FUNCREF or EXTERNREF)
};

//
// WASM Module Instance
//

struct ModuleInstance {
  // Memory instances (multiple memories in MVP+)
  std::vector<MemoryInstance> memories;

  // Table instances
  std::vector<TableInstance> tables;

  // Global variables
  std::vector<TypedValue> globals;

  // Exported functions
  struct Export {
    char name[256];
    uint32_t index;      // Function/memory/table/global index
    enum { FUNC, TABLE, MEMORY, GLOBAL } kind;
  };
  std::vector<Export> exports;

  // Imported functions
  struct Import {
    char module[256];
    char name[256];
    enum { FUNC, TABLE, MEMORY, GLOBAL } kind;
    uint32_t index;
  };
  std::vector<Import> imports;

  // Data segments (for initializing memory)
  struct DataSegment {
    uint32_t memory_index;
    uint32_t offset;
    std::vector<uint8_t> data;
  };
  std::vector<DataSegment> data_segments;

  // Element segments (for initializing tables)
  struct ElementSegment {
    uint32_t table_index;
    uint32_t offset;
    std::vector<uint32_t> function_indices;
  };
  std::vector<ElementSegment> element_segments;
};

//
// WASM Instruction Opcodes (subset)
//

namespace Opcodes {
  // Control flow
  constexpr uint8_t UNREACHABLE   = 0x00;
  constexpr uint8_t NOP           = 0x01;
  constexpr uint8_t BLOCK         = 0x02;
  constexpr uint8_t LOOP          = 0x03;
  constexpr uint8_t IF            = 0x04;
  constexpr uint8_t ELSE          = 0x05;
  constexpr uint8_t END           = 0x0B;
  constexpr uint8_t BR            = 0x0C;  // Branch
  constexpr uint8_t BR_IF         = 0x0D;  // Branch if
  constexpr uint8_t BR_TABLE      = 0x0E;  // Branch table
  constexpr uint8_t RETURN        = 0x0F;
  constexpr uint8_t CALL          = 0x10;
  constexpr uint8_t CALL_INDIRECT = 0x11;

  // Parametric
  constexpr uint8_t DROP          = 0x1A;
  constexpr uint8_t SELECT        = 0x1B;

  // Variable access
  constexpr uint8_t LOCAL_GET     = 0x20;
  constexpr uint8_t LOCAL_SET     = 0x21;
  constexpr uint8_t LOCAL_TEE     = 0x22;
  constexpr uint8_t GLOBAL_GET    = 0x23;
  constexpr uint8_t GLOBAL_SET    = 0x24;

  // Memory operations
  constexpr uint8_t I32_LOAD      = 0x28;
  constexpr uint8_t I64_LOAD      = 0x29;
  constexpr uint8_t F32_LOAD      = 0x2A;
  constexpr uint8_t F64_LOAD      = 0x2B;
  constexpr uint8_t I32_LOAD8_S   = 0x2C;
  constexpr uint8_t I32_LOAD8_U   = 0x2D;
  constexpr uint8_t I32_STORE     = 0x36;
  constexpr uint8_t I64_STORE     = 0x37;
  constexpr uint8_t F32_STORE     = 0x38;
  constexpr uint8_t F64_STORE     = 0x39;
  constexpr uint8_t MEMORY_SIZE   = 0x3F;
  constexpr uint8_t MEMORY_GROW   = 0x40;

  // Constants
  constexpr uint8_t I32_CONST     = 0x41;
  constexpr uint8_t I64_CONST     = 0x42;
  constexpr uint8_t F32_CONST     = 0x43;
  constexpr uint8_t F64_CONST     = 0x44;

  // Comparison (i32)
  constexpr uint8_t I32_EQZ       = 0x45;
  constexpr uint8_t I32_EQ        = 0x46;
  constexpr uint8_t I32_NE        = 0x47;
  constexpr uint8_t I32_LT_S      = 0x48;
  constexpr uint8_t I32_LT_U      = 0x49;
  constexpr uint8_t I32_GT_S      = 0x4A;
  constexpr uint8_t I32_GT_U      = 0x4B;
  constexpr uint8_t I32_LE_S      = 0x4C;
  constexpr uint8_t I32_LE_U      = 0x4D;
  constexpr uint8_t I32_GE_S      = 0x4E;
  constexpr uint8_t I32_GE_U      = 0x4F;

  // Arithmetic (i32)
  constexpr uint8_t I32_CLZ       = 0x67;
  constexpr uint8_t I32_CTZ       = 0x68;
  constexpr uint8_t I32_POPCNT    = 0x69;
  constexpr uint8_t I32_ADD       = 0x6A;
  constexpr uint8_t I32_SUB       = 0x6B;
  constexpr uint8_t I32_MUL       = 0x6C;
  constexpr uint8_t I32_DIV_S     = 0x6D;
  constexpr uint8_t I32_DIV_U     = 0x6E;
  constexpr uint8_t I32_REM_S     = 0x6F;
  constexpr uint8_t I32_REM_U     = 0x70;
  constexpr uint8_t I32_AND       = 0x71;
  constexpr uint8_t I32_OR        = 0x72;
  constexpr uint8_t I32_XOR       = 0x73;
  constexpr uint8_t I32_SHL       = 0x74;
  constexpr uint8_t I32_SHR_S     = 0x75;
  constexpr uint8_t I32_SHR_U     = 0x76;
  constexpr uint8_t I32_ROTL      = 0x77;
  constexpr uint8_t I32_ROTR      = 0x78;

  // Threading (threads proposal)
  constexpr uint8_t ATOMIC_PREFIX = 0xFE;
  constexpr uint8_t MEMORY_ATOMIC_NOTIFY    = 0x00;  // After prefix
  constexpr uint8_t MEMORY_ATOMIC_WAIT32    = 0x01;
  constexpr uint8_t MEMORY_ATOMIC_WAIT64    = 0x02;
  constexpr uint8_t ATOMIC_FENCE            = 0x03;

  // SIMD (SIMD proposal)
  constexpr uint8_t SIMD_PREFIX   = 0xFD;
  constexpr uint8_t V128_LOAD     = 0x00;  // After prefix
  constexpr uint8_t V128_STORE    = 0x0B;
  constexpr uint8_t V128_CONST    = 0x0C;
}

//
// WASM Runtime Types
//

enum class RuntimeType {
  BROWSER,      // Running in web browser (V8, SpiderMonkey, JavaScriptCore)
  WASMTIME,     // Wasmtime standalone runtime
  WASMER,       // Wasmer runtime
  WAMR,         // WebAssembly Micro Runtime
  WAVM,         // WAVM runtime
  WASM3,        // WASM3 interpreter
  CUSTOM,       // Custom runtime
};

//
// Breakpoint Support
//

struct Breakpoint {
  uint32_t function_index;
  uint32_t offset;         // Instruction offset within function
  bool is_enabled;
};

//
// Source Mapping (for DWARF debug info)
//

struct SourceLocation {
  char filename[512];
  uint32_t line;
  uint32_t column;
  uint32_t function_index;
  uint32_t offset;
};

inline void clear() {}

} // namespace WASM
} // namespace Architecture
} // namespace ds2
