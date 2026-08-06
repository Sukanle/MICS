# 静态反射与工具 API 参考

本文档涵盖编译期静态反射系统（`static/`）和共享工具（`utils/`）。

---

## 1. 工具库（`utils/`）

### 1.1 `hash.h` — FNV-1a 哈希函数

**命名空间：** `Reflect::Utils`

| 符号 | 类型 | 说明 |
|--------|------|-------------|
| `hash64_t` | `uint64_t` | 64 位哈希类型别名 |
| `hash32_t` | `uint32_t` | 32 位哈希类型别名 |
| `fnv1a(s, n)` | `constexpr hash64_t` | 对字节缓冲区 `s`（长度为 `n`）进行 FNV-1a 哈希 |
| `cstr64(s)` | `constexpr hash64_t` | 对以 null 结尾的 C 字符串进行 FNV-1a 哈希 |
| `cstr32(s)` | `constexpr hash32_t` | 对 C 字符串进行 32 位截断 FNV-1a 哈希 |
| `mix(a, b)` | `constexpr hash64_t` | 哈希组合：`a ^ (b + MIX + (a << 6) + (a >> 2))` |
| `version(v)` | `constexpr hash64_t` | `cstr64(v)` 的别名，用于版本字符串 |

**调用约定标签：**

```cpp
namespace Reflect::Utils::cc {
    enum class tag : uint8_t {
        Cdecl      = 0,
        Stdcall    = 1,
        Fastcall   = 2,
        Vectorcall = 3
    };
}
```

**使用示例：**
```cpp
constexpr auto h = Utils::cstr64("MyType");           // 编译期哈希
constexpr auto combined = Utils::mix(h, Utils::cstr64("::field")); // 组合哈希
```

---

### 1.2 `type_hash.h` — 编译期类型 → 哈希映射

**命名空间：** `Reflect::Utils`

| 符号 | 类型 | 说明 |
|--------|------|-------------|
| `type_hash<T>()` | `constexpr hash64_t` | 为类型 `T` 生成唯一的编译期哈希值 |
| `type_tag<T>` | struct | 用户可通过 `STATIC_TYPE_TAG(T, tag)` 宏扩展的标签 |
| `STATIC_TYPE_TAG(T, tag)` | 宏 | 为用户类型 `T` 注册自定义哈希标签 |

**内置类型标签：**
`void`、`bool`、`char`、`int8_t`、`uint8_t`、`int16_t`、`uint16_t`、`int32_t`、`uint32_t`、`long`、`unsigned long`、`int64_t`、`uint64_t`、`float`、`double`、`long double`、`char8_t`、`char16_t`、`char32_t`、`wchar_t`

**指针/引用派生规则：**
- `T*` → `mix(cstr64("p"), type_hash<T>)`
- `T&` → `mix(cstr64("l"), type_hash<T>)`
- `T&&` → `mix(cstr64("r"), type_hash<T>)`

**自定义类型注册：**
```cpp
struct MyStruct { int x; };
STATIC_TYPE_TAG(MyStruct, "MyStruct");
// type_hash<MyStruct>() 现在返回 cstr64("MyStruct")
```

---

### 1.3 `vector.h` — ABI 稳定不可变容器

**命名空间：** `Reflect::Utils`

`Utils::vector<T>` 是专为动态反射数据载体设计的 ABI 稳定容器，具有以下强制约束：

| 约束 | 实现 |
|------|------|
| 禁止默认构造 | `vector() = delete` |
| 禁止拷贝 | 拷贝构造/赋值均 `= delete` |
| 仅允许移动 | 移动构造/赋值（用于返回值） |
| 唯一合法来源 | 通过 `vector_builder<T>` 友元工厂构建 |
| 不可修改 | 所有成员函数均为 `const`，不提供修改接口 |
| 外部持有/序列化 | 只能通过 `.to_std()` 转换为 `std::vector` |

**ABI 保证（相同模板参数 `T` 下）：**
- `sizeof(vector<T>)` == 24 字节（64 位）/ 12 字节（32 位），永久不变
- 内部使用 `malloc`/`free`，完全脱离 `std::allocator`

**主要 API：**

| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `size()` | `std::size_t` | 元素个数 |
| `capacity()` | `std::size_t` | 分配容量 |
| `empty()` | `bool` | 是否为空 |
| `operator[](i)` | `const T&` | 下标访问（无边界检查） |
| `at(i)` | `const T&` | 下标访问（有边界检查） |
| `front()` | `const T&` | 第一个元素 |
| `back()` | `const T&` | 最后一个元素 |
| `data()` | `const T*` | 原始指针 |
| `begin()` / `end()` | `const T*` | 迭代器（const-only） |
| `to_std()` | `std::vector<T>` | 转换为标准 `std::vector` |

**`vector_builder<T>`** — 可变构建器，唯一合法的 `vector` 构造来源：

| 方法 | 说明 |
|--------|-------------|
| `push_back(val)` | 追加元素 |
| `emplace_back(args...)` | 原位构造元素 |
| `reserve(n)` | 预留容量 |
| `build() &&` | 消费 builder，返回不可变 `vector` |

**工厂函数：**

| 函数 | 说明 |
|----------|-------------|
| `make_vector(src)` | 从 `std::vector<T>` 构建不可变 `vector` |

**使用示例：**
```cpp
// 通过 builder 构建
Utils::vector_builder<int> b;
b.push_back(1);
b.push_back(2);
b.push_back(3);
Utils::vector<int> v = std::move(b).build();

// 只读访问
for (std::size_t i = 0; i < v.size(); ++i) {
    std::printf("%d\n", v[i]);
}

// 转换为 std::vector 进行外部操作
std::vector<int> sv = v.to_std();
sv.push_back(4);  // 对 std::vector 的修改不影响原始 v
```

---

## 2. 静态反射（`static/`）

### 2.1 `config.h` — 核心类型定义

**命名空间：** `Reflect::Static`

| 符号 | 类型 | 说明 |
|--------|------|-------------|
| `template_depth` | `uint8_t` | 模板递归深度计数器别名 |
| `template_constants` | `uint8_t` | 模板常量值别名 |
| `type_list<Args...>` | struct | 异构类型列表，`count = sizeof...(Args)` |
| `empty_list` | 类型别名 | `type_list<>` |
| `is_type_list_of<T>` | trait | 若 `T` 是 `type_list<...>`，则 `::value` 为 `true` |
| `is_virtual_base_of<Base, Derived>` | trait | 检测虚基类关系 |
| `MAX_TEMPLATE_DEPTH` | 宏 | 默认：`std::numeric_limits<template_depth>::max()` |
| `MAX_TEMPLATE_CONSTANTS` | 宏 | 默认：`std::numeric_limits<template_constants>::max()` |

---

### 2.2 `template_string.h` — 非类型模板字符串

**命名空间：** `Reflect::Static`

编译期字符串作为非类型模板参数（C++20 `__cpp_nontype_template_args`）。

| 宏 | 说明 |
|-------|-------------|
| `SKL_DEFAULT_TEMPLATE_STRING(name, str)` | 带默认值声明 |
| `SKL_NORMAL_TEMPLATE_STRING(name)` | 不带默认值声明 |
| `SKL_MAKE_TEMPLATE_STRING(str)` | 将字符串字面量作为模板参数传递 |
| `SKL_TEMPLATE_STRING_SUPPORTED` | 编译器支持时为 `1`，否则为 `0` |

**回退方案：** 当 `SKL_TEMPLATE_STRING_SUPPORTED == 0` 时，使用 `const char*` 并以 `nullptr` 为默认值。

```cpp
template<SKL_NORMAL_TEMPLATE_STRING(Name)>
struct my_trait {
    static constexpr auto name = NameAccessor<Name>();
};
// 用法：my_trait<SKL_MAKE_TEMPLATE_STRING("hello")>
```

---

### 2.3 `fp.h` / `base_fp.h` — 类型列表函数式编程

**命名空间：** `Reflect::Static::Fp`

一个编译期函数式编程库，操作对象为 `type_list<...>`。

#### 元素访问

| 操作 | 签名 | 说明 |
|-----------|-----------|-------------|
| `nth<List, N>` | `using = type` | 第 N 个元素（从 0 开始） |
| `head<List>` | `using = type` | 第一个元素 |
| `tail<List>` | `using = type` | 最后一个元素 |
| `other<List>` | `using = type_list<...>` | 除第一个外的所有元素 |

#### 修改操作

| 操作 | 签名 | 说明 |
|-----------|-----------|-------------|
| `push_front<List, T>` | `using = type_list<T, Args...>` | 在头部插入 `T` |
| `push_back<List, T>` | `using = type_list<Args..., T>` | 在尾部追加 `T` |
| `pop_front<List>` | `using = type_list<...>` | 移除第一个元素 |
| `pop_back<List>` | `using = type_list<...>` | 移除最后一个元素 |
| `concat<Lists...>` | `using = type_list<...>` | 连接多个列表 |
| `remove<List, Target>` | `using = type_list<...>` | 移除所有 `Target` |

#### 查询与统计

| 操作 | 签名 | 说明 |
|-----------|-----------|-------------|
| `size<List>` | `constexpr template_constants` | 元素个数 |
| `count<List, F>` | `constexpr template_constants` | 统计满足 `F<T>::value` 的元素个数 |
| `find_index<List, Target>` | `constexpr signed` | `Target` 的索引，未找到返回 `-1` |

#### 高阶操作

| 操作 | 签名 | 说明 |
|-----------|-----------|-------------|
| `map<List, F, T>` | `using = type_list<...>` | 将满足 `F<T>::value` 的元素替换为 `T` |
| `transform<F, List>` | `using = type_list<...>` | 对每个元素应用 `F<T>::type` |
| `flat_map<F, List>` | `using = type_list<...>` | 应用 `F<T>::type` 并展平 |
| `filter<List, F>` | `using = type_list<...>` | 保留满足 `F<T>::value` 的元素 |
| `filter_args<List, F, Arg>` | `using = type_list<...>` | 带额外参数的过滤 |
| `fold<List, Init, Func>` | `using = type` | 使用 `Func<Acc, T>::type` 左折叠 |
| `unique<List>` | `using = type_list<...>` | 去重，保留首次出现 |

**使用示例：**
```cpp
using List = type_list<int, char, double, int>;
using Filtered = Fp::filter<List, std::is_integral>;    // type_list<int, char, int>
using Unique = Fp::unique<Filtered>;                      // type_list<int, char>
constexpr auto n = Fp::size<Unique>;                      // 2
```

---

### 2.4 `var_traits.h` — 变量/字段萃取

**命名空间：** `Reflect::Static`

| 符号 | 说明 |
|--------|-------------|
| `var_type<T>` | 基础：`type = T`，`var_ptr = T*` |
| `var_type<T Class::*>` | 成员指针：`m_var_ptr = T Class::*`，`class_t = Class`，`type = T` |
| `__base_var_traits<T, Name>` | `name`、`is_const`、`is_volatile` |
| `var_traits<T, Name>` | 自由/静态变量：`is_member = false` |
| `var_traits<T Class::*, Name>` | 成员变量：`is_member = false`（指针以值形式存储） |

---

### 2.5 `fn_traits.h` — 函数/方法萃取

**命名空间：** `Reflect::Static`

| 符号 | 说明 |
|--------|-------------|
| `fn_type<Signature>` | 提取 `ret_t`、`class_t`、`args_t`（作为 `std::tuple`） |
| `__base_fn_traits<Signature, Class, Name>` | `name`、`is_member`、`params_count`、`hash` |
| `fn_traits<Signature, Class, Name>` | 完整萃取，含 `fn_ptr`/`m_fn_ptr`、`modifie` 位掩码 |

**`fn_qualify` 位掩码常量：**

| 常量 | 值 | 含义 |
|----------|-------|---------|
| `NOTHING` | `0x00` | 无修饰符 |
| `NOEXCEPT` | `0x01` | `noexcept` |
| `CONST` | `0x02` | `const` |
| `VOLATILE` | `0x04` | `volatile` |
| `CV` | `0x06` | `const volatile` |
| `LVALUE` | `0x08` | `&`（左值引用限定） |
| `RVALUE` | `0x10` | `&&`（右值引用限定） |
| `CC_MASK` | `0x60` | 调用约定掩码 |
| `CDECL` | `0x00` | `__cdecl` |
| `STDCALL` | `0x20` | `__stdcall` |
| `FASTCALL` | `0x40` | `__fastcall` |
| `VECTORCALL` | `0x60` | `__vectorcall` |

**宏辅助：**
- `SREFL_FNT_HELP(fn, ...)` — 解析成员函数指针类型，支持可选的类上下文

**使用示例：**
```cpp
using Traits = fn_traits<decltype(&MyClass::method), MyClass, SKL_MAKE_TEMPLATE_STRING("method")>;
static_assert(Traits::is_member);
static_assert(Traits::params_count == 2);
constexpr auto h = Traits::hash;   // 唯一的签名哈希值
```

---

### 2.6 `enum_traits.h` — 枚举萃取

**命名空间：** `Reflect::Static`

| 符号 | 说明 |
|--------|-------------|
| `is_scoped_enum<E>` | 对于 `enum class`，`::value` 为 `true` |
| `is_normal_enum<E>` | 对于普通 `enum`，`::value` 为 `true` |
| `enum_type<E>` | `type = E`，`underlying_t = std::underlying_type_t<E>` |
| `enum_traits<E>` | `list`（静态 `std::map<E, string_view>`）、`is_scoped` |

**使用示例：**
```cpp
enum class Color { Red, Green, Blue };
// 注册枚举值：
enum_traits<Color>::list = {
    {Color::Red, "Red"},
    {Color::Green, "Green"},
    {Color::Blue, "Blue"}
};
static_assert(enum_traits<Color>::is_scoped);
```

---

### 2.7 `base_reflect.h` — 统一字段萃取

**命名空间：** `Reflect::Static`

| 符号 | 说明 |
|--------|-------------|
| `Kind` | `enum`：`FreeOrStatic_Var`、`FreeOrStatic_Fn`、`NonStaticMem_Var`、`NonStaticMem_Fn` |
| `is_Kind<T>()` | `consteval Kind` — 分类指针类型 |
| `__base_field_traits<T, Kind, Class, Name>` | 所有字段类型的统一基类 |

**`__base_field_traits` 成员（所有特化的并集）：**
- `is_member()`、`is_function()`、`is_variable()`、`is_static()`
- `is_const()`、`is_volatile()`、`is_lvalue()`、`is_rvalue()`、`is_noexcept()`
- `params_count()`（仅函数）
- `_ptr` — 实际的指针值

---

### 2.8 `reflect.h` — 公共 API

**命名空间：** `Reflect::Static`

| 符号 | 说明 |
|--------|-------------|
| `NOT<F, Args...>` | `constexpr bool` — 对 `F<Args...>::value` 取反 |
| `TypeInfo<T>` | 主模板（为每个已注册类型特化） |
| `is_type_list<T>` | trait：若 `T` 是 `type_list`，则 `::value` 为 `true` |
| `cv_combinations<T>` / `ref_combinations<T>` | 生成 cv 限定/引用限定变体的 `type_list` |
| `strip_prefix(name, prefix)` | 从 `string_view` 中移除前缀（无模板字符串时的回退方案） |
| `field_traits<T, Class, Name>` | 面向用户的字段萃取（继承自 `__base_field_traits`） |
| `type_info<T>()` | `consteval` — 返回 `TypeInfo<T>{}` |

**`field_traits` 成员：**
- `getName()` — 运行时名称访问
- `from_TempName()` — 来自模板参数的 `consteval` 名称
- `_ptr` — 指向字段/方法的实际指针

**使用示例：**
```cpp
constexpr auto ft = field_traits<decltype(&Person::name), Person, SKL_MAKE_TEMPLATE_STRING("name")>{&Person::name};
auto name = ft.getName();                       // "name"
auto tmp = ft.from_TempName();                  // "name"（编译期）

constexpr auto info = type_info<Person>();
// info 包含 .fields、.methods、.bases 成员列表
```

---

## 3. 注册宏（静态反射）

### 简化宏（推荐使用）

| 宏 | 说明 |
|---|------|
| `RFS_CLASS(ClassName)` | 开始对象注册，声明 `class_t` |
| `SKL_RFS_PROPERTY(member)` | 注册成员（默认自动注册） |
| `RFS_CLASS_END()` | 结束注册块 |

### 旧版宏（向后兼容）

| 宏 | 说明 |
|-------|-------------|
| `SKL_RFS_OBJ_BEGIN(ClassName)` | 开始对象注册，声明 `class_t` |
| `RFS_OBJ_MEM(member)` | 注册成员（默认自动注册） |
| `RFS_OBJ_REG(member)` | 显式注册成员 |
| `RFS_OBJ_REG_TEM(member)` | 基于模板的显式注册 |
| `SKL_RFS_OBJ_END()` | 结束注册块 |
| `ENABLE_REFLECT_SKIP` | 定义后禁用 `RFS_OBJ_MEM` 的自动注册 |
| `REFLECT_DEFAULT_REGISTER` / `RELECT_DEFAULT_REGISTER` | 在 `ENABLE_REFLECT_SKIP` 模式下恢复默认注册 |

### 映射关系

| 简化宏 | 旧版等价宏 |
|--------|-----------|
| `RFS_CLASS(T)` | `SKL_RFS_OBJ_BEGIN(T)` |
| `SKL_RFS_PROPERTY(m)` | `RFS_OBJ_MEM(m)` |
| `RFS_CLASS_END()` | `SKL_RFS_OBJ_END()` |

---

## 4. 全局别名

在 `reflect.h`（根目录）中定义：

```cpp
namespace SRefl = Reflect::Static;
namespace DRefl = Reflect::Dynamic;
```

这些提供了便捷的简写：
- `SRefl::type_info<T>()` — 静态反射入口
- `DRefl::Registry::instance()` — 动态反射入口