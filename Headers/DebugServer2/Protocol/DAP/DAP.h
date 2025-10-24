//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// DAP (Debug Adapter Protocol)
//
// DAP is a protocol by Microsoft for editor-agnostic debugging
// Used by VS Code, Visual Studio, and many other editors/IDEs
//

#pragma once

#include "DebugServer2/Base.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace ds2 {
namespace Protocol {
namespace DAP {

//
// DAP Overview
//
// Debug Adapter Protocol is a JSON-based protocol for debugging:
// - Created by Microsoft for VS Code
// - Editor/IDE agnostic
// - Supports multiple languages and debuggers
// - Two-way communication over stdin/stdout or sockets
// - Request/Response and Event model
// - Language-agnostic primitives (threads, stackframes, variables, etc.)
//

//
// JSON Types (simplified representations)
//

using JSONValue = std::string;  // Simplified - would use a JSON library in practice
using JSONObject = std::map<std::string, JSONValue>;
using JSONArray = std::vector<JSONValue>;

//
// Base Message Types
//

enum class MessageType {
  REQUEST,
  RESPONSE,
  EVENT,
};

struct Message {
  uint32_t seq;             // Sequence number
  MessageType type;         // Message type
  std::string message;      // Message content
};

//
// Request Message
//

struct Request : public Message {
  std::string command;      // Command name
  JSONObject arguments;     // Command arguments
};

//
// Response Message
//

struct Response : public Message {
  uint32_t request_seq;     // Sequence of corresponding request
  bool success;             // Success/failure
  std::string command;      // Command name
  std::string message;      // Error message (if !success)
  JSONValue body;           // Response body
};

//
// Event Message
//

struct Event : public Message {
  std::string event;        // Event type
  JSONValue body;           // Event body
};

//
// DAP Request Commands
//

namespace Commands {
  // Initialization
  constexpr const char* INITIALIZE         = "initialize";
  constexpr const char* CONFIGURATION_DONE = "configurationDone";
  constexpr const char* LAUNCH             = "launch";
  constexpr const char* ATTACH             = "attach";
  constexpr const char* DISCONNECT         = "disconnect";
  constexpr const char* TERMINATE          = "terminate";

  // Breakpoints
  constexpr const char* SET_BREAKPOINTS           = "setBreakpoints";
  constexpr const char* SET_FUNCTION_BREAKPOINTS  = "setFunctionBreakpoints";
  constexpr const char* SET_EXCEPTION_BREAKPOINTS = "setExceptionBreakpoints";
  constexpr const char* SET_DATA_BREAKPOINTS      = "setDataBreakpoints";
  constexpr const char* SET_INSTRUCTION_BREAKPOINTS = "setInstructionBreakpoints";

  // Execution control
  constexpr const char* CONTINUE           = "continue";
  constexpr const char* NEXT               = "next";
  constexpr const char* STEP_IN            = "stepIn";
  constexpr const char* STEP_OUT           = "stepOut";
  constexpr const char* STEP_BACK          = "stepBack";
  constexpr const char* REVERSE_CONTINUE   = "reverseContinue";
  constexpr const char* PAUSE              = "pause";

  // Stack and threads
  constexpr const char* THREADS            = "threads";
  constexpr const char* STACK_TRACE        = "stackTrace";
  constexpr const char* SCOPES             = "scopes";
  constexpr const char* VARIABLES          = "variables";

  // Evaluation
  constexpr const char* EVALUATE           = "evaluate";
  constexpr const char* SET_VARIABLE       = "setVariable";
  constexpr const char* SET_EXPRESSION     = "setExpression";

  // Source
  constexpr const char* SOURCE             = "source";
  constexpr const char* LOADED_SOURCES     = "loadedSources";

  // Modules
  constexpr const char* MODULES            = "modules";

  // Advanced
  constexpr const char* RESTART            = "restart";
  constexpr const char* RESTART_FRAME      = "restartFrame";
  constexpr const char* GOTO               = "goto";
  constexpr const char* GOTO_TARGETS       = "gotoTargets";
  constexpr const char* COMPLETIONS        = "completions";
  constexpr const char* EXCEPTION_INFO     = "exceptionInfo";

  // Memory
  constexpr const char* READ_MEMORY        = "readMemory";
  constexpr const char* WRITE_MEMORY       = "writeMemory";
  constexpr const char* DISASSEMBLE        = "disassemble";

  // Cancellation
  constexpr const char* CANCEL             = "cancel";
}

//
// DAP Event Types
//

namespace Events {
  // Initialization
  constexpr const char* INITIALIZED        = "initialized";
  constexpr const char* CAPABILITIES       = "capabilities";

  // Execution
  constexpr const char* STOPPED            = "stopped";
  constexpr const char* CONTINUED          = "continued";
  constexpr const char* EXITED             = "exited";
  constexpr const char* TERMINATED         = "terminated";

  // Threads
  constexpr const char* THREAD             = "thread";

  // Output
  constexpr const char* OUTPUT             = "output";

  // Breakpoints
  constexpr const char* BREAKPOINT         = "breakpoint";

  // Modules
  constexpr const char* MODULE             = "module";

  // Sources
  constexpr const char* LOADED_SOURCE      = "loadedSource";

  // Process
  constexpr const char* PROCESS            = "process";

  // Progress
  constexpr const char* PROGRESS_START     = "progressStart";
  constexpr const char* PROGRESS_UPDATE    = "progressUpdate";
  constexpr const char* PROGRESS_END       = "progressEnd";

  // Memory
  constexpr const char* MEMORY             = "memory";

  // Custom
  constexpr const char* CUSTOM             = "custom";
}

//
// Stop Reasons
//

namespace StopReasons {
  constexpr const char* STEP               = "step";
  constexpr const char* BREAKPOINT         = "breakpoint";
  constexpr const char* EXCEPTION          = "exception";
  constexpr const char* PAUSE              = "pause";
  constexpr const char* ENTRY              = "entry";
  constexpr const char* GOTO               = "goto";
  constexpr const char* FUNCTION_BREAKPOINT = "function breakpoint";
  constexpr const char* DATA_BREAKPOINT    = "data breakpoint";
  constexpr const char* INSTRUCTION_BREAKPOINT = "instruction breakpoint";
}

//
// DAP Data Types
//

struct Capabilities {
  bool supports_configuration_done_request = false;
  bool supports_function_breakpoints = false;
  bool supports_conditional_breakpoints = false;
  bool supports_hit_conditional_breakpoints = false;
  bool supports_evaluate_for_hovers = false;
  bool supports_step_back = false;
  bool supports_set_variable = false;
  bool supports_restart_frame = false;
  bool supports_goto_targets_request = false;
  bool supports_step_in_targets_request = false;
  bool supports_completions_request = false;
  bool supports_modules_request = false;
  bool supports_restart_request = false;
  bool supports_exception_options = false;
  bool supports_value_formatting_options = false;
  bool supports_exception_info_request = false;
  bool supports_terminate_debuggee = false;
  bool supports_delayed_stack_trace_loading = false;
  bool supports_loaded_sources_request = false;
  bool supports_log_points = false;
  bool supports_terminate_threads_request = false;
  bool supports_set_expression = false;
  bool supports_terminate_request = false;
  bool supports_data_breakpoints = false;
  bool supports_read_memory_request = false;
  bool supports_write_memory_request = false;
  bool supports_disassemble_request = false;
  bool supports_cancel_request = false;
  bool supports_clipboard_context = false;
  bool supports_stepping_granularity = false;
  bool supports_instruction_breakpoints = false;
  bool supports_exception_filter_options = false;
};

struct Thread {
  int64_t id;
  std::string name;
};

struct Source {
  std::string name;
  std::string path;
  int64_t source_reference = 0;
  std::string presentation_hint;  // "normal", "emphasize", "deemphasize"
  std::string origin;
  std::vector<Source> sources;
  JSONValue adapter_data;
  std::vector<std::string> checksums;
};

struct StackFrame {
  int64_t id;
  std::string name;
  Source source;
  int32_t line;
  int32_t column;
  int32_t end_line = 0;
  int32_t end_column = 0;
  bool can_restart = false;
  std::string instruction_pointer_reference;
  std::string module_id;
  std::string presentation_hint;  // "normal", "label", "subtle"
};

struct Scope {
  std::string name;
  std::string presentation_hint;  // "arguments", "locals", "registers"
  int64_t variables_reference;
  int32_t named_variables = 0;
  int32_t indexed_variables = 0;
  bool expensive;
  Source source;
  int32_t line = 0;
  int32_t column = 0;
  int32_t end_line = 0;
  int32_t end_column = 0;
};

struct Variable {
  std::string name;
  std::string value;
  std::string type;
  std::string presentation_hint;  // "kind", "attributes", "visibility"
  std::string evaluate_name;
  int64_t variables_reference = 0;
  int32_t named_variables = 0;
  int32_t indexed_variables = 0;
  JSONValue memory_reference;
};

struct Breakpoint {
  int64_t id = 0;
  bool verified;
  std::string message;
  Source source;
  int32_t line = 0;
  int32_t column = 0;
  int32_t end_line = 0;
  int32_t end_column = 0;
  std::string instruction_reference;
  int64_t offset = 0;
};

struct SourceBreakpoint {
  int32_t line;
  int32_t column = 0;
  std::string condition;
  std::string hit_condition;
  std::string log_message;
};

struct FunctionBreakpoint {
  std::string name;
  std::string condition;
  std::string hit_condition;
};

struct DataBreakpoint {
  std::string data_id;
  std::string access_type;  // "read", "write", "readWrite"
  std::string condition;
  std::string hit_condition;
};

struct InstructionBreakpoint {
  std::string instruction_reference;
  int64_t offset = 0;
  std::string condition;
  std::string hit_condition;
};

struct Module {
  std::string id;
  std::string name;
  std::string path;
  bool is_optimized = false;
  bool is_user_code = false;
  std::string version;
  std::string symbol_status;
  std::string symbol_file_path;
  std::string date_time_stamp;
  std::string address_range;
};

struct ExceptionDetails {
  std::string message;
  std::string type_name;
  std::string full_type_name;
  std::string evaluate_name;
  std::string stack_trace;
  std::vector<ExceptionDetails> inner_exception;
};

//
// DAP Session
//

class Session {
public:
  Session();
  ~Session();

  //
  // Connection management
  //

  ErrorCode startStdio();  // Use stdin/stdout
  ErrorCode startSocket(uint16_t port);  // Listen on TCP port
  ErrorCode stop();

  //
  // Message handling
  //

  ErrorCode receiveMessage(Message **message);
  ErrorCode sendResponse(const Response &response);
  ErrorCode sendEvent(const Event &event);

  //
  // Helper methods for common events
  //

  ErrorCode sendInitializedEvent();
  ErrorCode sendStoppedEvent(int64_t thread_id, const char *reason, const char *description = nullptr);
  ErrorCode sendContinuedEvent(int64_t thread_id);
  ErrorCode sendExitedEvent(int32_t exit_code);
  ErrorCode sendTerminatedEvent();
  ErrorCode sendThreadEvent(int64_t thread_id, const char *reason);  // "started" or "exited"
  ErrorCode sendOutputEvent(const char *category, const char *output);  // category: "console", "stdout", "stderr", "telemetry"
  ErrorCode sendBreakpointEvent(const char *reason, const Breakpoint &breakpoint);  // reason: "changed", "new", "removed"
  ErrorCode sendModuleEvent(const char *reason, const Module &module);  // reason: "new", "changed", "removed"

  //
  // Capabilities
  //

  void setCapabilities(const Capabilities &capabilities);
  const Capabilities& getCapabilities() const;

  //
  // State
  //

  bool isInitialized() const { return _initialized; }
  bool isConfigured() const { return _configured; }
  void setInitialized(bool init) { _initialized = init; }
  void setConfigured(bool configured) { _configured = configured; }

private:
  int _input_fd;            // Input file descriptor (stdin or socket)
  int _output_fd;           // Output file descriptor (stdout or socket)
  uint32_t _seq_number;     // Current sequence number
  Capabilities _capabilities;
  bool _initialized;
  bool _configured;

  ErrorCode readContent(char *buffer, size_t length);
  ErrorCode writeContent(const char *buffer, size_t length);
  ErrorCode parseMessage(const char *json, Message **message);
  ErrorCode serializeMessage(const Message &message, std::string &json);
};

//
// Helper Functions
//

// Convert between ds2 error codes and DAP error messages
const char* getDAPErrorMessage(ErrorCode error);

// Create response from request
Response createResponse(const Request &request, bool success, const char *message = nullptr);

// Create event
Event createEvent(const char *event_type);

} // namespace DAP
} // namespace Protocol
} // namespace ds2
