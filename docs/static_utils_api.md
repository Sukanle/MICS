# Static Reflection & Utils API Reference

This document covers the compile-time static reflection system (`ct/`) and shared utilities (`utils/`).

> **Namespace mapping:** `Reflect::Static` → `mics::ct` (alias `SRefl`), `Reflect::Utils` → `mics::utils` (alias `URefl`), `Reflect::Static::Fp` → `mics::ct::fp`

---

## 1. utils (`utils/`)

### 1.1 `hash.h` — FNV-1a Hash Functions

**Namespace:** `mics::utils` / `URefl`

| Symbol | Type | Description |
|--------|------|-------------|
| `hash64_t` | `uint64_t` | 64-bit hash type alias |
| `hash32_t` | `uint32_t` | 32-bit hash type alias |
| `fnv1a(s, n)` | `constexpr hash64_t` | FNV-1a hash of byte buffer `s` of length `n` |
| `cstr64(s)` | `constexpr hash64_t` | FNV-1a hash of null-terminated C string |
| `cstr32(s)` | `constexpr hash32_t` | 32-bit truncated FNV-1a hash of C string |
| `mix(a, b)` | `constexpr hash64_t` | Hash combine: `a ^ (b + MIX + (a << 6) + (a >> 2))` |
| `version(v)` | `constexpr hash64_t` | Alias for `cstr64(v)`, used for version strings |

**Calling Convention Tags:**

```cpp
namespace mics::utils::cc {
    enum class tag : uint8_t {
        Cdecl      = 0,
        Stdcall    = 1,
        Fastcall   = 2,
        Vectorcall = 3
    };
}
```

**Usage Example:**
```cpp
constexpr auto h = URefl::cstr64("MyType");           // compile-time hash
constexpr auto combined = URefl::mix(h, URefl::cstr64("::field")); // combine
```

---

### 1.2 `type_hash.h` — Compile-time Type → Hash

**Namespace:** `mics::utils`

| Symbol | Type | Description |
|--------|------|-------------|
| `type_hash<T>()` | `constexpr hash64_t` | Generate unique compile-time hash for type `T` |
| `type_tag<T>` | struct | User-extensible tag with `STATIC_TYPE_TAG(T, tag)` macro |
| `STATIC_TYPE_TAG(T, tag)` | macro | Register a custom hash tag for user type `T` |

**Built-in type tags:**
`void`, `bool`, `char`, `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `long`, `unsigned long`, `int64_t`, `uint64_t`, `float`, `double`, `long double`, `char8_t`, `char16_t`, `char32_t`, `wchar_t`

**Pointer/Reference derivation:**
- `T*` → `mix(cstr64("p"), type_hash<T>)`
- `T&` → `mix(cstr64("l"), type_hash<T>)`
- `T&&` → `mix(cstr64("r"), type_hash<T>)`

**Custom Type Registration:**
```cpp
struct MyStruct { int x; };
STATIC_TYPE_TAG(MyStruct, "MyStruct");
// type_hash<MyStruct>() now returns cstr64("MyStruct")
```

---

### 1.3 `vector.h` — ABI Immutable Container

**Namespace:** `mics::utils`

`URefl::vector<T>` is an ABI-stable container designed specifically as the data carrier for dynamic reflection, with the following mandatory constraints:

| Constraint | Implementation |
|------------|----------------|
| No default construction | `vector() = delete` |
| No copy | Copy constructor/assignment both `= delete` |
| Move-only | Move constructor/assignment (for return values) |
| Single legal source | Constructed via `vector_builder<T>` friend factory |
| Immutable | All member functions are `const`, no mutation interface |
| External holding/serialization | Only via `.to_std()` converting to `std::vector` |

**ABI guarantees (under same template parameter `T`):**
- `sizeof(vector<T>)` == 24 bytes (64-bit) / 12 bytes (32-bit), forever
- Uses `malloc`/`free` internally, completely decoupled from `std::allocator`

**Main API:**

| Method | Returns | Description |
|--------|---------|-------------|
| `size()` | `std::size_t` | Element count |
| `capacity()` | `std::size_t` | Allocated capacity |
| `empty()` | `bool` | Whether empty |
| `operator[](i)` | `const T&` | Subscript access (no bounds check) |
| `at(i)` | `const T&` | Subscript access (with bounds check) |
| `front()` | `const T&` | First element |
| `back()` | `const T&` | Last element |
| `data()` | `const T*` | Raw pointer |
| `begin()` / `end()` | `const T*` | Iterators (const-only) |
| `to_std()` | `std::vector<T>` | Convert to standard `std::vector` |

**`vector_builder<T>`** — Mutable builder, the only legal source for constructing `vector`:

| Method | Description |
|--------|-------------|
| `push_back(val)` | Append element |
| `emplace_back(args...)` | In-place construct element |
| `reserve(n)` | Reserve capacity |
| `build() &&` | Consume builder, return immutable `vector` |

**Factory functions:**

| Function | Description |
|----------|-------------|
| `make_vector(src)` | Build immutable `vector` from `std::vector<T>` |

**Usage Example:**
```cpp
// Build via builder
URefl::vector_builder<int> b;
b.push_back(1);
b.push_back(2);
b.push_back(3);
URefl::vector<int> v = std::move(b).build();

// Read-only access
for (std::size_t i = 0; i < v.size(); ++i) {
    std::printf("%d\n", v[i]);
}

// Convert to std::vector for external manipulation
std::vector<int> sv = v.to_std();
sv.push_back(4);  // modification to std::vector does not affect original v
```

---

### 1.4 `fn_hash.h` — Function-Signature Folding Hash

**Namespace:** `mics::utils`

| Symbol | Type | Description |
|--------|------|-------------|
| `compute_fn_hash<RetHash, ArgHashes...>()` | `constexpr hash64_t` | Fold a return-type hash plus a sequence of argument-type hashes into a stable function-signature hash |

The single source of truth for folding a function signature into a hash that deliberately excludes any calling-convention encoding. Both `mics::ct::fn_traits` and the ABIX cross-DLL reflection layer (`skl::abix::fn_sig`) reuse it:

- Callers pass their own compile-time type hashes as template arguments (`type_hash_v<Ret>` / `type_hash_v<Args>` for static reflection).
- It never encodes the calling convention; upper layers fold it in separately (e.g. ABIX mixes its `cc::tag` protocol code after the call).

**Usage Example:**
```cpp
constexpr auto h = URefl::compute_fn_hash<URefl::type_hash_v<int>, URefl::type_hash_v<int>>();
```

---

## 2. Static Reflection (`ct/`)

### 2.1 `config.h` — Core Type Definitions

**Namespace:** `mics::ct`

| Symbol | Type | Description |
|--------|------|-------------|
| `template_depth` | `uint8_t` | Alias for template recursion depth counter |
| `template_constants` | `uint8_t` | Alias for template constant values |
| `type_list<Args...>` | struct | Heterogeneous type list, `count = sizeof...(Args)` |
| `empty_list` | type alias | `type_list<>` |
| `is_type_list_of<T>` | trait | `::value` is `true` if `T` is a `type_list<...>` |
| `is_virtual_base_of<Base, Derived>` | trait | Detects virtual base class relationships |
| `MAX_TEMPLATE_DEPTH` | macro | Default: `std::numeric_limits<template_depth>::max()` |
| `MAX_TEMPLATE_CONSTANTS` | macro | Default: `std::numeric_limits<template_constants>::max()` |

---

### 2.2 `template_string.h` — Non-Type Template String

**Namespace:** `mics::ct`

Compile-time string as a non-type template parameter (C++20 `__cpp_nontype_template_args`).

| Macro | Description |
|-------|-------------|
| `SKL_DEFAULT_TEMPLATE_STRING(name, str)` | Declare with default value |
| `SKL_NORMAL_TEMPLATE_STRING(name)` | Declare without default |
| `SKL_MAKE_TEMPLATE_STRING(str)` | Pass string literal as template argument |
| `SKL_TEMPLATE_STRING_SUPPORTED` | `1` if compiler supports it, `0` otherwise |

**Fallback:** When `SKL_TEMPLATE_STRING_SUPPORTED == 0`, uses `const char*` with `nullptr` default.

```cpp
template<SKL_NORMAL_TEMPLATE_STRING(Name)>
struct my_trait {
    static constexpr auto name = NameAccessor<Name>();
};
// Usage: my_trait<SKL_MAKE_TEMPLATE_STRING("hello")>
```

---

### 2.3 `fp.h` / `base_fp.h` — Type List Functional Programming

**Namespace:** `mics::ct::fp`

A compile-time functional programming library operating on `type_list<...>`.

#### Element Access

| Operation | Signature | Description |
|-----------|-----------|-------------|
| `nth<List, N>` | `using = type` | N-th element (0-indexed) |
| `head<List>` | `using = type` | First element |
| `tail<List>` | `using = type` | Last element |
| `other<List>` | `using = type_list<...>` | All but first element |

#### Modification

| Operation | Signature | Description |
|-----------|-----------|-------------|
| `push_front<List, T>` | `using = type_list<T, Args...>` | Prepend `T` |
| `push_back<List, T>` | `using = type_list<Args..., T>` | Append `T` |
| `pop_front<List>` | `using = type_list<...>` | Remove first element |
| `pop_back<List>` | `using = type_list<...>` | Remove last element |
| `concat<Lists...>` | `using = type_list<...>` | Concatenate multiple lists |
| `remove<List, Target>` | `using = type_list<...>` | Remove all `Target` occurrences |

#### Query & Statistics

| Operation | Signature | Description |
|-----------|-----------|-------------|
| `size<List>` | `constexpr template_constants` | Element count |
| `count<List, F>` | `constexpr template_constants` | Count elements satisfying `F<T>::value` |
| `find_index<List, Target>` | `constexpr signed` | Index of `Target`, `-1` if not found |

#### Higher-Order

| Operation | Signature | Description |
|-----------|-----------|-------------|
| `map<List, F, T>` | `using = type_list<...>` | Replace elements where `F<T>::value` with `T` |
| `transform<F, List>` | `using = type_list<...>` | Apply `F<T>::type` to each element |
| `flat_map<F, List>` | `using = type_list<...>` | Apply `F<T>::type` and flatten |
| `filter<List, F>` | `using = type_list<...>` | Keep elements where `F<T>::value` |
| `filter_args<List, F, Arg>` | `using = type_list<...>` | Filter with additional argument |
| `fold<List, Init, Func>` | `using = type` | Left-fold with `Func<Acc, T>::type` |
| `unique<List>` | `using = type_list<...>` | Deduplicate, keep first occurrence |

**Usage Example:**
```cpp
using List = type_list<int, char, double, int>;
using Filtered = fp::filter<List, std::is_integral>;    // type_list<int, char, int>
using Unique = fp::unique<Filtered>;                      // type_list<int, char>
constexpr auto n = fp::size<Unique>;                      // 2
```

---

### 2.4 `var_traits.h` — Variable / Field Traits

**Namespace:** `mics::ct`

| Symbol | Description |
|--------|-------------|
| `var_type<T>` | Base: `type = T`, `var_ptr = T*` |
| `var_type<T Class::*>` | Member pointer: `m_var_ptr = T Class::*`, `class_t = Class`, `type = T` |
| `__base_var_traits<T, Name>` | `name`, `is_const`, `is_volatile` |
| `var_traits<T, Name>` | Free/static variable: `is_member = false` |
| `var_traits<T Class::*, Name>` | Member variable: `is_member = false` (pointer stored as value) |

---

### 2.5 `fn_traits.h` — Function / Method Traits

**Namespace:** `mics::ct`

| Symbol | Description |
|--------|-------------|
| `fn_type<Signature>` | Extracts `ret_t`, `class_t`, `args_t` (as `std::tuple`) |
| `__base_fn_traits<Signature, Class, Name>` | `name`, `is_member`, `params_count`, `hash` |
| `fn_traits<Signature, Class, Name>` | Full traits with `fn_ptr`/`m_fn_ptr`, `modifie` bitmask |

**`hash` source:** Each `__base_fn_traits` computes its `hash` via `mics::utils::compute_fn_hash<type_hash_v<Ret>, type_hash_v<Args>...>()`; it never encodes the calling convention.

**`fn_qualify` bitmask constants:**

| Constant | Value | Meaning |
|----------|-------|---------|
| `SREFL_NOTHING` | `0x00` | No qualifiers |
| `SREFL_NOEXCEPT` | `0x01` | `noexcept` |
| `SREFL_CONST` | `0x02` | `const` |
| `SREFL_VOLATILE` | `0x04` | `volatile` |
| `SREFL_CV` | `0x06` | `const volatile` |
| `SREFL_LVALUE` | `0x08` | `&` (lvalue-ref qualified) |
| `SREFL_RVALUE` | `0x10` | `&&` (rvalue-ref qualified) |

**Macro helpers:**
- `SREFL_FNT_HELP(fn, ...)` — Resolves member function pointer type with optional class context

**Usage Example:**
```cpp
using Traits = fn_traits<decltype(&MyClass::method), MyClass, SKL_MAKE_TEMPLATE_STRING("method")>;
static_assert(Traits::is_member);
static_assert(Traits::params_count == 2);
constexpr auto h = Traits::hash;   // unique signature hash
```

---

### 2.6 `enum_traits.h` — Enum Traits

**Namespace:** `mics::ct`

| Symbol | Description |
|--------|-------------|
| `is_scoped_enum<E>` | `::value` is `true` for `enum class` |
| `is_normal_enum<E>` | `::value` is `true` for plain `enum` |
| `enum_type<E>` | `type = E`, `underlying_t = std::underlying_type_t<E>` |
| `enum_traits<E>` | `list` (static `std::map<E, string_view>`), `is_scoped` |

**Usage Example:**
```cpp
enum class Color { Red, Green, Blue };
// Register enum values:
enum_traits<Color>::list = {
    {Color::Red, "Red"},
    {Color::Green, "Green"},
    {Color::Blue, "Blue"}
};
static_assert(enum_traits<Color>::is_scoped);
```

---

### 2.7 `base_reflect.h` — Unified Field Traits

**Namespace:** `mics::ct`

| Symbol | Description |
|--------|-------------|
| `Kind` | `enum`: `FreeOrStatic_Var`, `FreeOrStatic_Fn`, `NonStaticMem_Var`, `NonStaticMem_Fn` |
| `is_Kind<T>()` | `consteval Kind` — classify a pointer type |
| `__base_field_traits<T, Kind, Class, Name>` | Unified base for all field types |

**`__base_field_traits` members (union of all specializations):**
- `is_member()`, `is_function()`, `is_variable()`, `is_static()`
- `is_const()`, `is_volatile()`, `is_lvalue()`, `is_rvalue()`, `is_noexcept()`
- `params_count()` (functions only)
- `_ptr` — the actual pointer value

---

### 2.8 `reflect.h` — Public API

**Namespace:** `mics::ct`

| Symbol | Description |
|--------|-------------|
| `NOT<F, Args...>` | `constexpr bool` — negate `F<Args...>::value` |
| `TypeInfo<T>` | Primary template (specialized for each registered type) |
| `is_type_list<T>` | Trait: `::value` is `true` for `type_list` |
| `cv_combinations<T>` / `ref_combinations<T>` | Generate `type_list` of cv-qualified / ref-qualified variants |
| `strip_prefix(name, prefix)` | Remove prefix from `string_view` (fallback when no template string) |
| `field_traits<T, Class, Name>` | User-facing field traits (inherits `__base_field_traits`) |
| `type_info<T>()` | `consteval` — returns `TypeInfo<T>{}` |

**`field_traits` members:**
- `getName()` — runtime name access
- `from_TempName()` — `consteval` name from template parameter
- `_ptr` — the actual pointer to the field/method

**Usage Example:**
```cpp
constexpr auto ft = field_traits<decltype(&Person::name), Person, SKL_MAKE_TEMPLATE_STRING("name")>{&Person::name};
auto name = ft.getName();                       // "name"
auto tmp = ft.from_TempName();                  // "name" (compile-time)

constexpr auto info = type_info<Person>();
// info has .fields, .methods, .bases member lists
```

---

## 3. Registration Macros (Static)

### Simplified Macros (Recommended)

| Macro | Description |
|-------|-------------|
| `RFS_CLASS(ClassName)` | Begin object registration, declares `class_t` |
| `SKL_RFS_PROPERTY(member)` | Register a member (auto-register by default) |
| `RFS_CLASS_END()` | End registration block |

### Legacy Macros (Backward Compatible)

| Macro | Description |
|-------|-------------|
| `SKL_RFS_OBJ_BEGIN(ClassName)` | Begin object registration, declares `class_t` |
| `RFS_OBJ_MEM(member)` | Register a member (auto-register by default) |
| `RFS_OBJ_REG(member)` | Explicitly register a member |
| `RFS_OBJ_REG_TEM(member)` | Template-based explicit registration |
| `SKL_RFS_OBJ_END()` | End registration block |
| `ENABLE_REFLECT_SKIP` | Define to disable auto-registration via `RFS_OBJ_MEM` |
| `REFLECT_DEFAULT_REGISTER` / `RELECT_DEFAULT_REGISTER` | Restore default registration in `ENABLE_REFLECT_SKIP` mode |

### Mapping

| Simplified | Legacy Equivalent |
|------------|-------------------|
| `RFS_CLASS(T)` | `SKL_RFS_OBJ_BEGIN(T)` |
| `SKL_RFS_PROPERTY(m)` | `RFS_OBJ_MEM(m)` |
| `RFS_CLASS_END()` | `SKL_RFS_OBJ_END()` |

---

## 4. User Aliases

Defined in `mics.h` (root):

```cpp
namespace SRefl = ::mics::ct;     // Compile-time static reflection
namespace DRefl = ::mics::rt;     // Runtime dynamic reflection
namespace URefl = ::mics::utils;   // ABI-stable utility container
```

These provide convenient shorthand:
- `SRefl::type_info<T>()` — static reflection entry
- `DRefl::Registry::instance()` — dynamic reflection entry
- `URefl::vector<T>` — ABI-stable container
- `URefl::string_view` — lightweight string view