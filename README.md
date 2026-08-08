<div align="center">

# Reflection

## A modern C++17 (and later) reflection library providing both compile-time (static) and runtime (dynamic) type introspection.

![Liencese](https://img.shields.io/badge/Liencese-Apache_2.0-blue)
![Language](https://img.shields.io/badge/Language-C/C++-red)
![Version](https://img.shields.io/badge/Version-1.0.0-green)

English | [中文](README_ZH.md)
</div>

## Features

- **Static Reflection** — Compile-time type introspection, template metaprogramming, and zero-overhead field/method iteration via `consteval`/`constexpr`
- **Dynamic Reflection** — Runtime type registration, type-erased field access, and method invocation with a global registry
- **SKL_ABIX-Stable Container** — Immutable, move-only `Utils::vector<T>` with fixed memory layout, cross-version binary compatibility, designed as the data carrier for dynamic reflection
- **Type Hashing** — FNV-1a based compile-time type hashing, enabling cross-boundary type identification (e.g., DLL/SO hot-reload)
- **Functional-Type Programming** — Compile-time type list manipulation (`map`, `filter`, `fold`, `flat_map`, `unique`, etc.)

## Quick Start

```cpp
#include "reflect.h"

// --- Static Reflection ---
struct Person {
    std::string name;
    int age = 0;
    void greet() const { std::printf("Hello, I'm %s\n", name.c_str()); }
};

// Register fields and methods at compile time
SKL_RFS_CLASS(Person)
    SKL_RFS_PROPERTY(name)
    SKL_RFS_PROPERTY(age)
    SKL_RFS_PROPERTY(greet)
SKL_RFS_CLASS()

// Iterate over registered members at compile time
constexpr auto info = SRefl::type_info<Person>();
// info.fields, info.methods, etc.

// --- Dynamic Reflection ---
SKL_RFD_CLASS(Person)
    SKL_RFD_PROPERTY(name)
    SKL_RFD_PROPERTY(age)
    SKL_RFD_METHOD(greet)
SKL_RFD_CLASS()

// Query at runtime
auto *ti = DRefl::Registry::instance().find_by_name("Person");
auto *field = ti->find_field("name");
field->setter(&obj, &new_value);    // type-erased field write
```

> [!NOTE]
> - [Static Reflection API.md](doc/static_utils_api_zh_CN.md)
> - [Dynamic Reflection API.md](doc/dynamic_utils_api_zh_CN.md)

## Registration Macros

The library provides two tiers of registration macros:

### Simplified Macros (Recommended)

These are the concise, unified entry points for everyday use:

| Macro | Description |
|-------|-------------|
| `SKL_RFS_CLASS(ClassName)` | Begin static class registration |
| `SKL_RFS_PROPERTY(member)` | Register a static field or method |
| `SKL_RFS_CLASS()` | End static class registration |
| `SKL_RFS_ENUM(EnumName)` | Begin static enum registration |
| `SKL_RFS_ENUM_VALUE(value, name)` | Register a static enum value |
| `SKL_RFS_ENUM()` | End static enum registration |
| `SKL_RFD_CLASS(ClassName)` | Begin dynamic class registration |
| `SKL_RFD_PROPERTY(member)` | Register a dynamic property |
| `SKL_RFD_METHOD(method)` | Register a dynamic method |
| `SKL_RFD_CLASS()` | End dynamic class registration |
| `SKL_RFD_ENUM(EnumName)` | Begin dynamic enum registration |
| `SKL_RFD_ENUM_VALUE(value, name)` | Register a dynamic enum value |
| `SKL_RFD_ENUM()` | End dynamic enum registration |

### Legacy Macros (Backward Compatible)

The original verbose macros remain available internally:

| Simplified | Legacy Equivalent |
|------------|-------------------|
| `SKL_RFS_CLASS(T)` | `SKL_RFS_REGISTER_BEGIN(T)` |
| `SKL_RFS_PROPERTY(m)` | `SKL_RFS_PROPERTY(m)` |
| `SKL_RFS_CLASS()` | `SKL_RFS_REGISTER_END()` |
| `SKL_RFS_ENUM(T)` | `SKL_RFS_ENUM_BEGIN(T)` |
| `SKL_RFS_ENUM_VALUE(v, n)` | `SKL_RFS_ENUM_VALUE(v, n)` |
| `SKL_RFS_ENUM()` | `SKL_RFS_ENUM_END()` |
| `SKL_RFD_CLASS(T)` | `SKL_RFD_REGISTER_BEGIN(T)` |
| `SKL_RFD_PROPERTY(m)` | `SKL_RFD_FIELD(m)` |
| `SKL_RFD_METHOD(m)` | `SKL_RFD_METHOD(m)` |
| `SKL_RFD_CLASS()` | `SKL_RFD_REGISTER_END()` |
| `SKL_RFD_ENUM(T)` | `SKL_RFD_ENUM_BEGIN(T)` |
| `SKL_RFD_ENUM_VALUE(v, n)` | `SKL_RFD_ENUM_VALUE(v, n)` |
| `SKL_RFD_ENUM()` | `SKL_RFD_ENUM_END()` |

## Metadata System (`metadata.h`)

The metadata system provides a stable SKL_ABIX through fixed-layout structures and external string tables.

### Metadata Modes

| Mode | Value | Description |
|------|-------|-------------|
| `SKL_REFT_MODE_HASH` | `0` | Hash-only, no strings — Release / minimal deployment |
| `SKL_REFT_MODE_STR` | `1` | Essential strings — Debug / development |
| `SKL_REFT_MODE_FULL` | `2` | Full debug metadata — Editor / IDE integration |

Default strategy: Debug → `STR`, Release → `HASH`. Override with `-DSKL_REFT_META_MODE=0|1|2`.

### Key Types

```cpp
// Fixed SKL_ABIX structure — layout never changes
struct MetaEntry {
    uint64_t id;              // FNV1a64 hash
    uint32_t flags;           // MetaFlags bitmask
    uint32_t metadataIndex;   // Index into external string table
};

// External string table — safe across DLL boundaries
struct MetaStringTable {
    uint32_t count;
    uint32_t dataSize;
    const StringEntry* entries;
    const char* data;
};
```

### Constexpr Hash

```cpp
constexpr uint64_t id = SKL_REFT_HASH("Player.Health");   // FNV1a64, cross-compiler stable
constexpr uint32_t id32 = SKL_REFT_HASH32("Player.Health");
```

## Directory Structure

```
Refection/
├── reflect.h              # Main entry header (includes static + dynamic)
├── metadata.h             # Metadata system (MetaEntry, MetaMode, SKL_REFT_HASH, StringTable)
├── static/                # Compile-time static reflection
│   ├── reflect.h          # Public API & field_traits, TypeInfo
│   ├── base_reflect.h     # __base_field_traits (var/fn dispatch)
│   ├── base_fp.h          # Low-level type list operations
│   ├── fp.h               # Public type list API (Fp namespace)
│   ├── var_traits.h       # Variable/field traits
│   ├── fn_traits.h        # Function/method traits (qualifiers, hash)
│   ├── enum_traits.h      # Enum traits & scoped enum detection
│   ├── config.h           # type_list, template_depth, is_virtual_base_of
│   └── template_string.h  # Non-type template parameter string support
├── dynamic/               # Runtime dynamic reflection
│   ├── reflect.h          # Public API, registration macros, type_id_of
│   ├── config.h           # TypeId, Kind, FieldKind, Visibility enums
│   ├── type_info.h        # TypeInfo: aggregate descriptor (fields/methods/bases)
│   ├── field_info.h       # FieldInfo + FieldAccessor (getter/setter)
│   ├── fn_info.h          # FnInfo + MethodInvoker + ParamInfo
│   ├── enum_info.h        # EnumInfo + EnumEntry
│   ├── any.h              # Any: type-erased value container (SBO)
│   └── registry.h         # Registry: global singleton type registry
├── utils/                 # Shared utilities
│   ├── hash.h             # FNV-1a hash functions, calling convention tags
│   ├── type_hash.h        # Compile-time type → hash mapping
│   ├── fn_hash.h          # Function-signature folding hash (compute_fn_hash)
│   ├── string_view.h      # Lightweight string_view implementation
│   └── vector.h           # SKL_ABIX-stable immutable container (dynamic reflection data carrier)
├── doc/                   # API documentation
└── HLMD/                  # Macro utility library (internal)
```

## Supported Platforms & Toolchains

| Platform | Compiler | Minimum Version | Status |
|----------|----------|-----------------|--------|
| Windows  | MSVC     | VS 2022 (17.0+) | ✓ |
| Windows  | MinGW-w64 (GCC) | 13.0+ | ✓ |
| Windows  | Clang-cl | 17.0+ | ✓ |
| Linux    | GCC      | 13.0+ | ✓ |
| Linux    | Clang    | 17.0+ | ✓ |

**Requirements:** C++17 or later (C++20 recommended for `consteval` and `__cpp_nontype_template_args`; concepts support recommended)

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2), driven by `main.cpp` covering:

| Test Category | Tags | Coverage |
|--------------|------|----------|
| Static class reflection | `[static][class]` | Type metadata, field read/write, method invocation for `Player` and `Weapon` (incl. custom template names and const methods) |
| Static enum reflection | `[static][enum]` | Value lists and `is_scoped` detection for `Fruit` (unscoped) and `Direction` (scoped `enum class`) |
| Dynamic class reflection | `[dynamic][class]` | Type lookup by name via `Registry`, `FieldAccessor` getter/setter, `FnInfo` method lookup |
| Dynamic enum reflection | `[dynamic][enum]` | `EnumInfo` entry iteration, `is_scoped` flag |
| Registry queries | `[dynamic][registry]` | `find_by_name`, `type_count`, `type_at` iteration |
| Integration consistency | `[integration]` | Verifies field/method count and name consistency between static and dynamic reflection for the same type |

```bash
# Build and run tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## Future Plans

- **MOC (Meta Object Compiler)** — Planned to be built on Clang LibTooling, using C++ attributes (`[[...]]`) to annotate classes and members, automatically generating static and dynamic reflection registration code. Developers simply add attributes like `[[meta::export]]`, `[[meta::property]]`, `[[meta::method]]` to classes or fields, and the MOC tool will scan source files and emit the corresponding registration code — no manual macros needed.

## License

Apache License, Version 2.0. See [LICENSE](./LICENSE) for the full text.

---

Copyright 2026 [Sukanle](https://github.com/Sukanle)