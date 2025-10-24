//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
// PowerPC-AS for IBM AS/400 (now IBM i)
//
// PowerPC-AS (Application Specific) processors for AS/400/iSeries/IBM i
// systems. These processors implement IMPI (Integrated Multi-Processing
// Infrastructure) with TIMI (Technology Independent Machine Interface).
//

#pragma once

#include <cstdint>

namespace ds2 {
namespace Architecture {
namespace PowerPC {
namespace AS400 {

//
// AS/400 PowerPC-AS Overview
//
// AS/400 systems use special PowerPC processors with additional features:
// 1. IMPI (Integrated Multi-Processing Infrastructure) - Hardware support
// 2. TIMI (Technology Independent Machine Interface) - Translation layer
// 3. MI (Machine Interface) - High-level object-based instruction set
// 4. Single-level storage - All memory is persistent storage
// 5. Object-based architecture - Everything is a typed, tagged object
// 6. Hardware-enforced security and object access control
//
// The MI instructions are translated to PowerPC instructions by TIMI,
// which can be implemented in hardware, firmware, or a combination.
//

//
// AS/400 PowerPC-AS Processor Variants
//

enum class AS400Processor {
  // 64-bit IMPI processors (1995-2000s)
  AMAZON,           // Amazon - First 64-bit AS/400 PowerPC processor
  APACHE,           // Apache - Enhanced Amazon
  NORTHSTAR,        // Northstar - Improved Apache with better performance
  PULSAR,           // Pulsar - Advanced IMPI processor
  ISTAR,            // iStar - Further evolution
  SSTAR,            // SStar - System i processor

  // POWER-based AS/400 (2000s onward - standard POWER with special firmware)
  POWER4_AS400,     // POWER4 with AS/400 SLIC firmware
  POWER5_AS400,     // POWER5 with i5/OS
  POWER5_PLUS_AS400, // POWER5+ with i5/OS
  POWER6_AS400,     // POWER6 with IBM i 6.1
  POWER7_AS400,     // POWER7 with IBM i 7.1
  POWER8_AS400,     // POWER8 with IBM i 7.2/7.3
  POWER9_AS400,     // POWER9 with IBM i 7.3/7.4/7.5
  POWER10_AS400,    // POWER10 with IBM i 7.5
};

//
// IMPI-Specific Features
//

struct IMPIFeatures {
  bool has_mi_translation;      // MI to PowerPC translation (TIMI)
  bool has_object_tagging;      // Hardware object type tags
  bool has_single_level_store;  // Single-level storage (no separate disk/memory)
  bool has_hardware_isolation;  // Hardware domain isolation
  bool has_capability_based;    // Capability-based addressing
  bool has_64bit_addresses;     // 64-bit address space (from Amazon onward)
};

//
// Object Types in AS/400
//
// All data in AS/400 is stored as typed objects with hardware enforcement
//

enum class ObjectType : uint16_t {
  // System objects
  OBJ_POINTER          = 0x0001,  // Pointer object
  OBJ_SPACE            = 0x0002,  // Space object (unstructured storage)
  OBJ_PROGRAM          = 0x0201,  // Program object
  OBJ_MODULE           = 0x0202,  // Module object
  OBJ_PROCEDURE        = 0x0203,  // Procedure object
  OBJ_SERVICE_PROGRAM  = 0x0204,  // Service program (shared library)

  // Data objects
  OBJ_DTAARA           = 0x0A01,  // Data area
  OBJ_DTAQ             = 0x0A02,  // Data queue
  OBJ_FILE             = 0x0A03,  // Database file
  OBJ_MSGQ             = 0x0A04,  // Message queue
  OBJ_JOBQ             = 0x0A05,  // Job queue
  OBJ_OUTQ             = 0x0A06,  // Output queue

  // Context objects
  OBJ_LIB              = 0x0401,  // Library (directory of objects)
  OBJ_DIR              = 0x0402,  // Directory (IFS)
  OBJ_CTXT             = 0x0403,  // Context object

  // User interface
  OBJ_MENU             = 0x0B01,  // Menu
  OBJ_DSPF             = 0x0B02,  // Display file
  OBJ_PRTF             = 0x0B03,  // Printer file

  // Security
  OBJ_USRPRF           = 0x0C01,  // User profile
  OBJ_AUTL             = 0x0C02,  // Authorization list

  // Configuration
  OBJ_CLS              = 0x0D01,  // Class object
  OBJ_JOBD             = 0x0D02,  // Job description
};

//
// MI (Machine Interface) Instruction Set
//
// High-level instructions above PowerPC ISA, translated by TIMI
//

namespace MI {
  // MI opcodes are 8-16 bits, very different from PowerPC
  // These are conceptual - actual encoding is proprietary

  enum class InstructionCategory {
    // Scalar operations
    CAT_SCALAR_INTEGER,       // Integer arithmetic
    CAT_SCALAR_FLOAT,         // Floating-point arithmetic
    CAT_SCALAR_DECIMAL,       // Packed decimal arithmetic

    // Pointer and object operations
    CAT_POINTER,              // Pointer manipulation
    CAT_OBJECT,               // Object operations
    CAT_SPACE,                // Space operations

    // Control flow
    CAT_BRANCH,               // Branch operations
    CAT_CALL,                 // Call/return operations

    // Data movement
    CAT_COPY,                 // Copy operations
    CAT_COMPARE,              // Compare operations

    // String operations
    CAT_STRING,               // String manipulation

    // System operations
    CAT_SYSTEM,               // System-level operations
    CAT_EXCEPTION,            // Exception handling
  };

  // Common MI operations (simplified names)
  namespace Operations {
    // Pointer operations
    constexpr uint16_t SETSPPO   = 0x1000;  // Set space pointer
    constexpr uint16_t SETSPPFP  = 0x1001;  // Set space pointer from pointer
    constexpr uint16_t CPYBYTES  = 0x1002;  // Copy bytes
    constexpr uint16_t CMPBYTES  = 0x1003;  // Compare bytes

    // Object operations
    constexpr uint16_t CRTS      = 0x2000;  // Create space
    constexpr uint16_t DESS      = 0x2001;  // Destroy space
    constexpr uint16_t MATS      = 0x2002;  // Materialize space attributes
    constexpr uint16_t MODS      = 0x2003;  // Modify space attributes

    // Arithmetic
    constexpr uint16_t ADDN      = 0x3000;  // Add numeric
    constexpr uint16_t SUBN      = 0x3001;  // Subtract numeric
    constexpr uint16_t MULN      = 0x3002;  // Multiply numeric
    constexpr uint16_t DIVN      = 0x3003;  // Divide numeric

    // Decimal arithmetic
    constexpr uint16_t ADDD      = 0x3100;  // Add decimal
    constexpr uint16_t SUBD      = 0x3101;  // Subtract decimal
    constexpr uint16_t MULD      = 0x3102;  // Multiply decimal
    constexpr uint16_t DIVD      = 0x3103;  // Divide decimal

    // Control flow
    constexpr uint16_t CALLX     = 0x4000;  // Call external
    constexpr uint16_t CALLI     = 0x4001;  // Call internal
    constexpr uint16_t RTN       = 0x4002;  // Return
    constexpr uint16_t BRANCH    = 0x4003;  // Branch

    // String operations
    constexpr uint16_t SCANX     = 0x5000;  // Scan
    constexpr uint16_t CPYBLA    = 0x5001;  // Copy bytes left-adjusted
    constexpr uint16_t TRIML     = 0x5002;  // Trim left
    constexpr uint16_t TRIMR     = 0x5003;  // Trim right

    // System operations
    constexpr uint16_t LOCK      = 0x6000;  // Lock object
    constexpr uint16_t UNLOCK    = 0x6001;  // Unlock object
    constexpr uint16_t SIGNAL    = 0x6002;  // Signal exception
  }
}

//
// SLIC (System Licensed Internal Code)
//
// The OS/400 microkernel running at the lowest level
//

namespace SLIC {
  // SLIC runs below MI and manages:
  // - Object management
  // - Storage management
  // - Process management
  // - I/O operations
  // - Security enforcement

  // SLIC call interface (conceptual)
  enum class SystemCall : uint16_t {
    SLIC_CREATE_OBJECT     = 0x0001,
    SLIC_DESTROY_OBJECT    = 0x0002,
    SLIC_RESOLVE_OBJECT    = 0x0003,
    SLIC_MATERIALIZE       = 0x0004,
    SLIC_MODIFY            = 0x0005,
    SLIC_LOCK_OBJECT       = 0x0010,
    SLIC_UNLOCK_OBJECT     = 0x0011,
    SLIC_ALLOCATE_SPACE    = 0x0020,
    SLIC_DEALLOCATE_SPACE  = 0x0021,
    SLIC_CREATE_THREAD     = 0x0030,
    SLIC_TERMINATE_THREAD  = 0x0031,
    SLIC_SIGNAL_EXCEPTION  = 0x0040,
    SLIC_HANDLE_EXCEPTION  = 0x0041,
  };
}

//
// AS/400 Addressing
//
// 128-bit pointers containing object identity and offset
//

struct AS400Pointer {
  uint64_t object_id;     // Object identifier (48-bit actual + metadata)
  uint64_t offset;        // Offset within object
  uint16_t type;          // Object type
  uint16_t authority;     // Access authority bits
  uint32_t reserved;      // Reserved/metadata
};

// Authority bits for pointers
enum AuthorityBits : uint16_t {
  AUTH_NONE              = 0x0000,
  AUTH_OBJECT_MGMT       = 0x0001,  // Object management
  AUTH_OBJECT_EXIST      = 0x0002,  // Object existence
  AUTH_OBJECT_ALTER      = 0x0004,  // Alter object
  AUTH_OBJECT_REFERENCE  = 0x0008,  // Reference object
  AUTH_READ              = 0x0010,  // Read data
  AUTH_ADD               = 0x0020,  // Add data
  AUTH_UPDATE            = 0x0040,  // Update data
  AUTH_DELETE            = 0x0080,  // Delete data
  AUTH_EXECUTE           = 0x0100,  // Execute
  AUTH_EXCLUDE           = 0x0200,  // Exclude others
  AUTH_ALL               = 0x03FF,  // All authority
};

//
// Single-Level Storage
//
// All objects exist in a single 64-bit address space that persists
//

struct SingleLevelStore {
  uint64_t base_address;  // Base of single-level storage
  uint64_t size;          // Total size (terabytes on modern systems)

  // Permanent object table
  struct ObjectEntry {
    AS400Pointer pointer;
    ObjectType type;
    uint32_t size;
    uint16_t attributes;
    uint16_t lock_state;
    uint64_t owner;
    uint64_t create_time;
    uint64_t modify_time;
  };
};

//
// AS/400 CPU State Extensions
//

struct AS400State {
  // Standard PowerPC state is used underneath

  // IMPI-specific state
  AS400Pointer instruction_pointer;  // Current MI instruction (128-bit)
  AS400Pointer stack_pointer;        // Stack pointer (128-bit)
  AS400Pointer base_pointer;         // Base pointer (128-bit)

  // MI register file (implementation dependent)
  AS400Pointer mi_regs[32];          // MI general purpose registers

  // Object context
  uint64_t current_object_id;        // Currently executing object
  uint64_t current_process_id;       // Process ID
  uint64_t current_job_id;           // Job ID (AS/400 concept)
  uint64_t current_activation_group; // Activation group ID

  // Security context
  uint64_t user_profile_id;          // User profile object
  uint16_t authority_mask;           // Current authority bits

  // Exception state
  uint16_t exception_code;           // Last exception code
  AS400Pointer exception_location;   // Exception location (128-bit)
};

//
// AS/400 Exception Types
//

enum class AS400Exception : uint16_t {
  EXC_NONE                 = 0x0000,
  EXC_POINTER_NOT_SET      = 0x0201,
  EXC_POINTER_TYPE         = 0x0202,
  EXC_POINTER_SPEC         = 0x0203,
  EXC_OBJECT_DESTROYED     = 0x0204,
  EXC_OBJECT_DAMAGED       = 0x0205,
  EXC_AUTHORITY_VIOLATION  = 0x0206,
  EXC_DOMAIN_VIOLATION     = 0x0207,
  EXC_SCALAR_OVERFLOW      = 0x0301,
  EXC_DECIMAL_DATA         = 0x0302,
  EXC_DECIMAL_OVERFLOW     = 0x0303,
  EXC_DECIMAL_DIVIDE       = 0x0304,
  EXC_FLOAT_OVERFLOW       = 0x0305,
  EXC_FLOAT_UNDERFLOW      = 0x0306,
  EXC_FLOAT_DIVIDE_ZERO    = 0x0307,
  EXC_FLOAT_INVALID        = 0x0308,
  EXC_MI_INSTRUCTION       = 0x0401,
  EXC_MI_MACHINE_CHECK     = 0x0402,
};

//
// TIMI (Technology Independent Machine Interface) Translation
//

struct TIMIContext {
  bool hardware_translation;    // TIMI implemented in hardware vs firmware
  uint32_t translation_cache_size; // Size of MI-to-PowerPC translation cache
  uint64_t translation_hits;    // Cache hit statistics
  uint64_t translation_misses;  // Cache miss statistics

  // Translation control
  bool enable_profiling;        // Profile MI instruction execution
  bool enable_debugging;        // Debug mode with MI visibility
};

//
// Processor Version Register (PVR) values for AS/400 processors
//

namespace PVR {
  constexpr uint32_t AMAZON       = 0x00410000;  // Amazon processor
  constexpr uint32_t APACHE       = 0x00420000;  // Apache processor
  constexpr uint32_t NORTHSTAR    = 0x00430000;  // Northstar processor
  constexpr uint32_t PULSAR       = 0x00440000;  // Pulsar processor
  constexpr uint32_t ISTAR        = 0x00450000;  // iStar processor
  constexpr uint32_t SSTAR        = 0x00460000;  // SStar processor
}

//
// AS/400 Debug Support
//

struct AS400DebugInfo {
  bool mi_level_debug;          // Debug at MI level (vs PowerPC level)
  bool break_on_exception;      // Break on MI exceptions
  bool break_on_object_access;  // Break on specific object access
  AS400Pointer watchpoint;      // 128-bit pointer watchpoint

  // MI instruction breakpoint
  AS400Pointer mi_breakpoint;

  // Object access breakpoint
  uint64_t watched_object_id;
  uint16_t watched_operations;  // Bit mask of operations to watch
};

} // namespace AS400
} // namespace PowerPC
} // namespace Architecture
} // namespace ds2
