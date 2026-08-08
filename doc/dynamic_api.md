# Dynamic Reflection API Reference

This document covers the runtime dynamic reflection system (`dynamic/`).

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│  reflect.h (entry)                                       │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Registration Macros (simplified)                   │  │
│  │ SKL_RFD_CLASS / SKL_RFD_PROPERTY / SKL_RFD_METHOD / ...       │  │
│  │ (legacy: SKL_RFD_REGISTER_BEGIN / SKL_RFD_FIELD / ...)    │  │
│  └──────────────┬────────────────────────────────────┘  │
│                 │ populates                              │
│  ┌──────────────▼────────────────────────────────────┐  │
│  │ TypeInfo (descriptor)                              │  │
│  │  ├─ FieldAccessor[]  (FieldInfo + getter/setter)   │  │
│  │  ├─ FnInfo[]         (method metadata + invoker)   │  │
│  │  ├─ BaseInfo[]       (base class info)             │  │
│  │  └─ EnumInfo*        (enum values, if applicable)  │  │
│  └──────────────┬────────────────────────────────────┘  │
│                 │ registered to                          │
│  ┌──────────────▼────────────────────────────────────┐  │
│  │ Registry (singleton)                               │  │
│  │  find_by_id() / find_by_name() / for_each()        │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Any (type-erased value container)                  │  │
│  │  SBO ≤ 16 bytes, try_cast<T>()                     │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 1. `config.h` — Core Type Definitions

**Namespace:** `Reflect::Dynamic`

### Type Aliases

| Symbol | Type | Description |
|--------|------|-------------|
| `TypeId` | `uint64_t` | Unique type identifier (hash-based) |
| `FieldId` | `uint32_t` | Field identifier |
| `MethodId` | `uint32_t` | Method identifier |
| `INVALID_TYPE_ID` | `constexpr TypeId` | Sentinel value `0` |

### Enumerations

**`Kind`** — Type classification:
| Value | Description |
|-------|-------------|
| `Class` | `class` type |
| `Struct` | `struct` type |
| `Enum` | Enumeration type |
| `Primitive` | Built-in primitive |
| `Pointer` | Pointer type |
| `Reference` | Reference type |

**`FieldKind`** — Field/method classification:
| Value | Description |
|-------|-------------|
| `MemberVar` | Non-static member variable |
| `StaticVar` | Static member variable |
| `MemberFn` | Non-static member function |
| `StaticFn` | Static member function |

**`Visibility`** — Access level:
| Value | Description |
|-------|-------------|
| `Public` | Public access |
| `Protected` | Protected access |
| `Private` | Private access |

---

## 2. `field_info.h` — Field Descriptors & Accessors

**Namespace:** `Reflect::Dynamic`

### `FieldInfo`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Field name |
| `type_id` | `TypeId` | Hash of field type |
| `offset` | `uint32_t` | Byte offset within the class |
| `kind` | `FieldKind` | MemberVar / StaticVar / MemberFn / StaticFn |
| `visibility` | `Visibility` | Public / Protected / Private |

**Methods:**
| Method | Returns | Description |
|--------|---------|-------------|
| `is_function()` | `bool` | `kind == MemberFn \|\| kind == StaticFn` |
| `is_variable()` | `bool` | `kind == MemberVar \|\| kind == StaticVar` |
| `is_static()` | `bool` | `kind == StaticVar \|\| kind == StaticFn` |

### `FieldAccessor`

| Member | Type | Description |
|--------|------|-------------|
| `info` | `FieldInfo` | Field metadata |
| `getter` | `FieldGetter` | `void*(*)(void* obj)` — type-erased getter |
| `setter` | `FieldSetter` | `void(*)(void* obj, void* value)` — type-erased setter |

**Usage Example:**
```cpp
auto *field = type_info->find_field("age");
if (field && field->getter) {
    int *age = static_cast<int*>(field->getter(&obj));
    *age = 42;
    // or equivalently:
    int newAge = 30;
    field->setter(&obj, &newAge);
}
```

---

## 3. `fn_info.h` — Method Descriptors & Invokers

**Namespace:** `Reflect::Dynamic`

### `ParamInfo`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Parameter name |
| `type_id` | `TypeId` | Hash of parameter type |

### `FnInfo`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Method name |
| `return_type_id` | `TypeId` | Hash of return type |
| `params` | `Utils::vector<ParamInfo>` | Parameter list (ABI-stable, immutable container) |
| `invoker` | `MethodInvoker` | `void(*)(void* obj, void** args, void* result)` |
| `visibility` | `Visibility` | Access level |
| `is_const` | `bool` | Whether the method is `const`-qualified |
| `is_static` | `bool` | Whether the method is `static` |

### `MethodInvoker`

```cpp
using MethodInvoker = void (*)(void *obj, void **args, void *result);
```

- `obj`: pointer to the object instance (ignored for static methods)
- `args`: array of `void*` pointers, each pointing to an argument value
- `result`: `void*` pointing to caller-allocated result storage

**Usage Example:**
```cpp
auto *method = type_info->find_method("getMarried");
if (method && method->invoker) {
    Person other;
    void *args[] = { &other };
    bool success = false;
    method->invoker(&obj, args, &success);
}
```

---

## 4. `enum_info.h` — Enum Descriptors

**Namespace:** `Reflect::Dynamic`

### `EnumEntry`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `Utils::string_view` | Enum value name |
| `value` | `int64_t` | Enum value (as signed 64-bit integer) |

### `EnumInfo`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Enum type name |
| `type_id` | `TypeId` | Hash of the enum type |
| `underlying_type_id` | `TypeId` | Hash of the underlying integer type |
| `entries` | `Utils::vector<EnumEntry>` | All enum value entries (ABI-stable, immutable container) |
| `is_scoped` | `bool` | Whether it's `enum class` (scoped) |

**Methods:**
| Method | Returns | Description |
|--------|---------|-------------|
| `find_name(value)` | `const char*` | Lookup name by value, `nullptr` if not found |
| `find_value(name, out)` | `bool` | Lookup value by name, sets `out` and returns `true` if found |

**Usage Example:**
```cpp
auto *ei = type_info->enum_info;
if (ei) {
    auto *name = ei->find_name(2);        // "write" for Permission::write
    int64_t val;
    if (ei->find_value("read", val)) {    // val = 1
        // ...
    }
}
```

---

## 5. `type_info.h` — Type Descriptor

**Namespace:** `Reflect::Dynamic`

### `BaseInfo`

| Member | Type | Description |
|--------|------|-------------|
| `type_id` | `TypeId` | Hash of base class type |
| `name` | `const char*` | Base class name |
| `offset` | `int32_t` | Byte offset of base subobject in derived class (0 = first base) |

### `TypeInfo`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Type name |
| `type_id` | `TypeId` | Unique hash identifier |
| `kind` | `Kind` | Class / Struct / Enum / Primitive / Pointer / Reference |
| `size` | `size_t` | `sizeof(T)` |
| `bases` | `Utils::vector<BaseInfo>` | Direct base classes (ABI-stable, immutable container) |
| `fields` | `Utils::vector<FieldAccessor>` | Registered fields (ABI-stable, immutable container) |
| `methods` | `Utils::vector<FnInfo>` | Registered methods (ABI-stable, immutable container) |
| `enum_info` | `const EnumInfo*` | Pointer to enum info (non-null only when `kind == Enum`) |

**Methods:**
| Method | Returns | Description |
|--------|---------|-------------|
| `find_field(name)` | `const FieldAccessor*` | Find field by name, `nullptr` if not found |
| `find_method(name)` | `const FnInfo*` | Find method by name, `nullptr` if not found |
| `has_base(type_id)` | `bool` | Check if `type_id` is a direct base class |

**Usage Example:**
```cpp
auto *ti = Registry::instance().find_by_name("Person");
if (ti) {
    for (auto &f : ti->fields) {
        printf("Field: %s (offset=%u)\n", f.info.name, f.info.offset);
    }
    for (auto &m : ti->methods) {
        printf("Method: %s (params=%zu)\n", m.name, m.params.size());
    }
}
```

---

## 6. `any.h` — Type-Erased Value Container

**Namespace:** `Reflect::Dynamic`

`Any` is a lightweight type-erased value container similar to `std::any`, optimized for reflection use cases.

### Design

- **SBO (Small Buffer Optimization):** Values ≤ 16 bytes are stored inline in a union buffer, avoiding heap allocation
- **Larger objects:** Allocated on heap with function-pointer-based `Handler` (clone/destroy)
- **Type safety:** `type_id()` returns the `TypeId` hash of the contained type

### API

| Method | Returns | Description |
|--------|---------|-------------|
| `Any()` | — | Default constructor, empty |
| `Any(T&& value)` | — | Construct from any movable value |
| `Any(const Any&)` | — | Copy constructor (deep copy via clone handler) |
| `Any(Any&&)` | — | Move constructor (steals buffer) |
| `operator=(Any)` | `Any&` | Copy-and-swap assignment |
| `swap(Any&)` | `void` | Swap contents |
| `type_id()` | `TypeId` | Type hash of contained value |
| `has_value()` | `bool` | Whether the Any contains a value |
| `operator bool()` | `bool` | Same as `has_value()` |
| `raw_ptr()` | `void*` | Raw pointer to contained value (SBO or heap) |
| `try_cast<T>()` | `T*` | Cast to `T*` if type matches, `nullptr` otherwise |
| `try_cast<T>() const` | `const T*` | Const version |

**Usage Example:**
```cpp
Any a = 42;                         // int, SBO inline
Any b = std::string("hello");       // string, heap allocated

int *p = a.try_cast<int>();         // succeeds, *p == 42
float *q = a.try_cast<float>();     // fails, q == nullptr

auto tid = a.type_id();             // type_hash<int>()
```

---

## 7. `registry.h` — Global Type Registry

**Namespace:** `Reflect::Dynamic`

`Registry` is a singleton that collects all statically-registered `TypeInfo` instances.

### API

| Method | Returns | Description |
|--------|---------|-------------|
| `instance()` | `Registry&` | Get the singleton instance |
| `register_type(info)` | `void` | Register a `TypeInfo*` (no-op if duplicate `type_id`) |
| `register_callback(cb)` | `void` | Register an external callback, invoked immediately to inject reflection data |
| `invoke_callbacks()` | `void` | Trigger all registered callbacks (for deferred registration) |
| `find_by_id(id)` | `const TypeInfo*` | Lookup by `TypeId` hash |
| `find_by_name(name)` | `const TypeInfo*` | Lookup by type name string |
| `type_count()` | `size_t` | Number of registered types |
| `type_at(index)` | `const TypeInfo*` | Get type by index, `nullptr` if out of bounds |
| `for_each(fn)` | `void` | Iterate all registered types with callback `fn(const TypeInfo*)` |
| `clear()` | `void` | Remove all registered types |

**Usage Example:**
```cpp
auto &reg = Registry::instance();

// Find by name
if (auto *ti = reg.find_by_name("Person")) {
    // ...
}

// Iterate all
reg.for_each([](const TypeInfo *ti) {
    printf("Registered: %s (id=%llu)\n", ti->name, ti->type_id);
});
```

---

## 8. `reflect.h` — Registration Macros & Public API

**Namespace:** `Reflect::Dynamic`

### `type_id_of<T>()`

```cpp
template<typename T>
constexpr TypeId type_id_of() noexcept;
```

Returns the compile-time `TypeId` hash for type `T`. Delegates to `Utils::type_hash<T>()`.

### Registration Macros

#### Simplified Macros (Recommended)

```cpp
SKL_RFD_CLASS(ClassName)
    SKL_RFD_PROPERTY(member)                       // Register a member variable
    SKL_RFD_PROPERTY(member, SKL_REFT_META_STRING)     // With forced string storage
    SKL_RFD_METHOD(method)                         // Register a member function
    SKL_RFD_METHOD(method, SKL_REFT_META_STRING)       // With forced string storage
SKL_RFD_CLASS_END()
```

| Macro | Description |
|-------|-------------|
| `SKL_RFD_CLASS(ClassName)` | Begin class registration, creates `TypeInfo` |
| `SKL_RFD_PROPERTY(member)` | Register a non-static member variable (auto-generates getter/setter) |
| `SKL_RFD_PROPERTY(member, SKL_REFT_META_STRING)` | Register with forced string storage (override `SKL_REFT_META_MODE`) |
| `SKL_RFD_METHOD(method)` | Register a member function (records name, signature, and invoker) |
| `SKL_RFD_METHOD(method, SKL_REFT_META_STRING)` | Register with forced string storage |
| `SKL_RFD_CLASS_END()` | End class registration, registers with `Registry` |

#### Legacy Macros (Backward Compatible)

```cpp
SKL_RFD_REGISTER_BEGIN(ClassName)
    SKL_RFD_FIELD(member)          // Register a member variable
    SKL_RFD_METHOD(method)         // Register a member function
SKL_RFD_CLASS_END()                // or SKL_RFD_REGISTER_END()
```

| Simplified | Legacy Equivalent |
|------------|-------------------|
| `SKL_RFD_CLASS(T)` | `SKL_RFD_REGISTER_BEGIN(T)` |
| `SKL_RFD_PROPERTY(m)` | `SKL_RFD_FIELD(m)` |
| `SKL_RFD_CLASS_END()` | `SKL_RFD_REGISTER_END()` |

#### Enum Registration

```cpp
SKL_RFD_ENUM(EnumName)
    SKL_RFD_ENUM_VALUE(value, "Name1")
    SKL_RFD_ENUM_VALUE(value, "Name2")
SKL_RFD_ENUM_END()
```

| Macro | Description |
|-------|-------------|
| `SKL_RFD_ENUM(EnumName)` | Begin enum registration, creates `EnumInfo` and `TypeInfo` |
| `SKL_RFD_ENUM_VALUE(value, name)` | Register an enum value entry |
| `SKL_RFD_ENUM_END()` | End enum registration, finalizes registration |

> **Note:** All registration macros internally use `Utils::vector_builder` to construct data. After registration completes, data is converted to immutable `Utils::vector`, ensuring ABI stability.

### Internal Helpers (`detail` namespace)

| Function | Description |
|----------|-------------|
| `member_type<T Class::*>` | Extracts `type` and `class_type` from member pointer |
| `make_field_getter<auto mp>()` | Creates `FieldGetter` from member pointer (template param) |
| `make_field_setter<auto mp>()` | Creates `FieldSetter` from member pointer (template param); array members are copied via `memcpy` |

---

## 9. Complete Usage Example

```cpp
#include "reflect.h"

// ============================================================
// Define a class
// ============================================================
class Player {
public:
    std::string name;
    int level = 1;
    float health = 100.0f;

    void takeDamage(float amount) { health -= amount; }
    bool isAlive() const { return health > 0.0f; }
};

// ============================================================
// Register with dynamic reflection
// ============================================================
SKL_RFD_CLASS(Player)
    SKL_RFD_PROPERTY(name)
    SKL_RFD_PROPERTY(level)
    SKL_RFD_PROPERTY(health)
    SKL_RFD_METHOD(takeDamage)
    SKL_RFD_METHOD(isAlive)
SKL_RFD_CLASS_END()

// ============================================================
// Register an enum
// ============================================================
enum class ItemRarity { Common, Rare, Epic, Legendary };

SKL_RFD_ENUM(ItemRarity)
    SKL_RFD_ENUM_VALUE(Common, "Common")
    SKL_RFD_ENUM_VALUE(Rare, "Rare")
    SKL_RFD_ENUM_VALUE(Epic, "Epic")
    SKL_RFD_ENUM_VALUE(Legendary, "Legendary")
SKL_RFD_ENUM_END()

// ============================================================
// Runtime usage
// ============================================================
void runtime_example() {
    using namespace Reflect::Dynamic;

    // --- Query type info ---
    auto *ti = Registry::instance().find_by_name("Player");
    if (!ti) return;

    // --- Create and manipulate via reflection ---
    Player p;

    // Read field
    auto *nameField = ti->find_field("name");
    if (nameField && nameField->getter) {
        auto *namePtr = static_cast<std::string*>(nameField->getter(&p));
        *namePtr = "Hero";
    }

    // Write field
    auto *hpField = ti->find_field("health");
    if (hpField && hpField->setter) {
        float newHp = 75.0f;
        hpField->setter(&p, &newHp);
    }

    // Invoke method
    auto *method = ti->find_method("takeDamage");
    if (method && method->invoker) {
        float dmg = 25.0f;
        void *args[] = { &dmg };
        method->invoker(&p, args, nullptr);
    }

    // --- Enum lookup ---
    auto *eTi = Registry::instance().find_by_name("ItemRarity");
    if (eTi && eTi->enum_info) {
        auto *name = eTi->enum_info->find_name(2);  // "Epic"
        int64_t val;
        eTi->enum_info->find_value("Legendary", val); // val = 3
    }
}
```

---

## 10. Design Notes

1. **Static initialization:** Registration happens via global object constructors before `main()`. The `static bool` guard prevents duplicate registration.
2. **Type ID consistency:** `TypeId` uses `Utils::type_hash<T>()`, ensuring the same hash across static and dynamic reflection.
3. **Field accessor design:** `FieldGetter`/`FieldSetter` use non-capturing lambdas converted to function pointers. Member pointers are passed as template parameters (`template<auto mp>`) to enable this.
4. **Thread safety:** The `Registry` is not thread-safe for concurrent registration. All registration should happen during static initialization (single-threaded).
5. **SBO threshold:** The `Any` class uses a 16-byte inline buffer. Types larger than 16 bytes or non-trivially-copyable types are heap-allocated.
6. **ABI-stable container:** All reflection data (`bases`, `fields`, `methods`, `params`, `entries`) is stored in `Utils::vector<T>`. This container is immutable, move-only, uses `malloc`/`free` internally, and has a fixed `sizeof` of 24 bytes (64-bit), ensuring cross-version ABI compatibility. For external manipulation, call `.to_std()` to convert to `std::vector`.
7. **Registration callback interface:** External modules can inject reflection data via `Registry::register_callback()`. The callback receives a `Registry&` reference and can call `register_type()` within it. The system safely takes ownership of the data after the callback returns.