//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// WebAssembly Debugging Interface
//
// Provides debugging support for WASM modules across different runtimes
//

#pragma once

#include "DebugServer2/Base.h"
#include "DebugServer2/Architecture/WASM/State.h"

namespace ds2 {
namespace Host {
namespace WASM {

using namespace Architecture::WASM;

//
// WASM Instance Handle
//

using InstanceHandle = uint64_t;
using FunctionHandle = uint32_t;

//
// WASM Debugging Interface
//

class Debug {
public:
  //
  // Runtime control
  //

  static ErrorCode attachToRuntime(RuntimeType runtime, InstanceHandle *handle);
  static ErrorCode detach(InstanceHandle handle);

  //
  // Module control
  //

  static ErrorCode loadModule(InstanceHandle handle, const uint8_t *wasm_binary, size_t size);
  static ErrorCode unloadModule(InstanceHandle handle);
  static ErrorCode instantiateModule(InstanceHandle handle);

  //
  // Execution control
  //

  static ErrorCode callFunction(InstanceHandle handle, const char *function_name,
                                const TypedValue *args, uint32_t num_args,
                                TypedValue *returns, uint32_t *num_returns);
  static ErrorCode suspend(InstanceHandle handle);
  static ErrorCode resume(InstanceHandle handle);
  static ErrorCode step(InstanceHandle handle);  // Single-step one instruction

  //
  // State inspection
  //

  static ErrorCode getExecutionState(InstanceHandle handle, ExecutionState &state);
  static ErrorCode getModuleInstance(InstanceHandle handle, ModuleInstance &module);

  //
  // Stack inspection
  //

  static ErrorCode getValueStack(InstanceHandle handle, std::vector<TypedValue> &stack);
  static ErrorCode getCallStack(InstanceHandle handle, std::vector<ExecutionState::Frame> &frames);

  //
  // Variable access
  //

  static ErrorCode getLocal(InstanceHandle handle, uint32_t index, TypedValue &value);
  static ErrorCode setLocal(InstanceHandle handle, uint32_t index, const TypedValue &value);
  static ErrorCode getGlobal(InstanceHandle handle, uint32_t index, TypedValue &value);
  static ErrorCode setGlobal(InstanceHandle handle, uint32_t index, const TypedValue &value);

  //
  // Memory operations
  //

  static ErrorCode readMemory(InstanceHandle handle, uint32_t memory_index, uint32_t offset,
                              void *buffer, size_t length);
  static ErrorCode writeMemory(InstanceHandle handle, uint32_t memory_index, uint32_t offset,
                               const void *buffer, size_t length);
  static ErrorCode getMemorySize(InstanceHandle handle, uint32_t memory_index, uint32_t *pages);
  static ErrorCode growMemory(InstanceHandle handle, uint32_t memory_index, uint32_t delta_pages);

  //
  // Table operations
  //

  static ErrorCode getTableElement(InstanceHandle handle, uint32_t table_index, uint32_t element_index,
                                   void **element);
  static ErrorCode setTableElement(InstanceHandle handle, uint32_t table_index, uint32_t element_index,
                                   void *element);
  static ErrorCode getTableSize(InstanceHandle handle, uint32_t table_index, uint32_t *size);

  //
  // Breakpoints
  //

  static ErrorCode setBreakpoint(InstanceHandle handle, uint32_t function_index, uint32_t offset);
  static ErrorCode removeBreakpoint(InstanceHandle handle, uint32_t function_index, uint32_t offset);
  static ErrorCode enumerateBreakpoints(InstanceHandle handle, std::vector<Breakpoint> &breakpoints);

  //
  // Module inspection
  //

  struct FunctionInfo {
    uint32_t index;
    char name[256];         // Function name (from name section or export)
    uint32_t num_params;
    ValueType param_types[32];
    uint32_t num_returns;
    ValueType return_types[32];
    uint32_t num_locals;
    uint32_t code_size;     // Size of function body in bytes
    bool is_imported;
    char import_module[256];
    char import_name[256];
  };

  static ErrorCode enumerateFunctions(InstanceHandle handle, std::vector<FunctionInfo> &functions);
  static ErrorCode getFunctionInfo(InstanceHandle handle, uint32_t function_index, FunctionInfo &info);
  static ErrorCode findFunction(InstanceHandle handle, const char *name, uint32_t *function_index);

  //
  // Source-level debugging (requires DWARF info)
  //

  static ErrorCode getSourceLocation(InstanceHandle handle, uint32_t function_index, uint32_t offset,
                                     SourceLocation &location);
  static ErrorCode findSourceLocation(InstanceHandle handle, const char *filename, uint32_t line,
                                      std::vector<SourceLocation> &locations);

  //
  // Exception handling (exception handling proposal)
  //

  struct Exception {
    uint32_t tag_index;
    std::vector<TypedValue> values;
  };

  static ErrorCode getException(InstanceHandle handle, Exception &exception);
  static ErrorCode throwException(InstanceHandle handle, uint32_t tag_index,
                                  const TypedValue *values, uint32_t num_values);

  //
  // Performance profiling
  //

  struct ProfileInfo {
    uint32_t function_index;
    uint64_t call_count;
    uint64_t total_time_ns;
    uint64_t self_time_ns;
  };

  static ErrorCode startProfiling(InstanceHandle handle);
  static ErrorCode stopProfiling(InstanceHandle handle);
  static ErrorCode getProfileInfo(InstanceHandle handle, std::vector<ProfileInfo> &profiles);

  //
  // Runtime information
  //

  struct RuntimeInfo {
    RuntimeType type;
    char version[128];
    bool supports_threads;
    bool supports_simd;
    bool supports_exceptions;
    bool supports_tail_calls;
    bool supports_bulk_memory;
    bool supports_reference_types;
    bool supports_multi_memory;
    uint32_t max_pages;      // Maximum memory pages supported
  };

  static ErrorCode getRuntimeInfo(InstanceHandle handle, RuntimeInfo &info);

  //
  // Import/Export inspection
  //

  static ErrorCode enumerateImports(InstanceHandle handle, std::vector<ModuleInstance::Import> &imports);
  static ErrorCode enumerateExports(InstanceHandle handle, std::vector<ModuleInstance::Export> &exports);
  static ErrorCode resolveImport(InstanceHandle handle, const char *module, const char *name,
                                 void *implementation);

  //
  // Memory tracing (for debugging memory bugs)
  //

  enum class TraceEvent {
    LOAD,
    STORE,
    GROW,
  };

  struct MemoryTrace {
    TraceEvent event;
    uint32_t memory_index;
    uint32_t offset;
    size_t length;
    uint32_t function_index;
    uint32_t instruction_offset;
  };

  static ErrorCode enableMemoryTracing(InstanceHandle handle, bool enable);
  static ErrorCode getMemoryTraces(InstanceHandle handle, std::vector<MemoryTrace> &traces);
};

//
// WASM Error Codes
//

namespace ErrorCodes {
  constexpr int WASM_OK                    = 0;
  constexpr int WASM_INVALID_MODULE        = -1;
  constexpr int WASM_VALIDATION_FAILED     = -2;
  constexpr int WASM_INSTANTIATION_FAILED  = -3;
  constexpr int WASM_TRAP                  = -4;   // Runtime trap occurred
  constexpr int WASM_UNREACHABLE           = -5;   // Unreachable instruction
  constexpr int WASM_INTEGER_OVERFLOW      = -6;
  constexpr int WASM_INTEGER_DIVIDE_BY_ZERO = -7;
  constexpr int WASM_INVALID_CONVERSION    = -8;
  constexpr int WASM_OUT_OF_BOUNDS_MEMORY  = -9;
  constexpr int WASM_OUT_OF_BOUNDS_TABLE   = -10;
  constexpr int WASM_INDIRECT_CALL_TYPE_MISMATCH = -11;
  constexpr int WASM_UNINITIALIZED_ELEMENT = -12;
  constexpr int WASM_CALL_STACK_EXHAUSTED  = -13;
  constexpr int WASM_RUNTIME_NOT_SUPPORTED = -14;
}

} // namespace WASM
} // namespace Host
} // namespace ds2
