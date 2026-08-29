<div align="center">

# MICS

## 元信息核心系统（Meta Information Core System）— 双轨（编译期 + 运行时）类型内省基础设施。

![Liencese](https://img.shields.io/badge/Liencese-Apache_2.0-blue)
![Language](https://img.shields.io/badge/Language-C/C++-red)
![Version](https://img.shields.io/badge/Version-1.0.0-green)

[English](README.md) | 中文 
</div>

MICS 在编译期提供零开销的静态反射（字段/方法遍历、类型列表函数式操作），在运行时提供基于全局注册表的动态类型查询（类型擦除访问器、方法调用器）。其底层以 ABI 稳定容器（`URefl::vector<T>`）承载元数据，确保跨 DLL/SO 边界二进制兼容。

### 用户别名

| 别名 | 命名空间 | 说明 |
|------|----------|------|
| `SRefl` | `mics::ct` | 编译期静态反射（Static Reflection） |
| `DRefl` | `mics::rt` | 运行时动态反射（Dynamic Reflection） |
| `URefl` | `mics::util` | ABI 稳定工具容器（Utility Reflection） |

## 特性

- **编译期静态反射**（`mics::ct` / `SRefl`）— 编译期类型内省、模板元编程，通过 `consteval`/`constexpr` 实现零开销的字段/方法遍历
- **运行时动态反射**（`mics::rt` / `DRefl`）— 运行时类型注册、类型擦除的字段访问以及带全局注册表的方法调用
- **ABI 稳定容器**（`URefl::vector<T>`）— 不可变、仅移动容器，固定内存布局，跨版本二进制兼容，专为动态反射数据载体设计
- **类型哈希** — 基于 FNV-1a 的编译期类型哈希，支持跨边界类型识别（如 DLL/SO 热重载）
- **函数式类型编程** — 编译期类型列表操作（`map`、`filter`、`fold`、`flat_map`、`unique` 等）

## 快速开始

```cpp
#include "mics.h"

// --- 静态反射 ---
struct Person {
    std::string name;
    int age = 0;
    void greet() const { std::printf("Hello, I'm %s\n", name.c_str()); }
};

// 在编译期注册字段和方法
SKL_RFS_CLASS(Person)
    SKL_RFS_PROPERTY(name)
    SKL_RFS_PROPERTY(age)
    SKL_RFS_PROPERTY(greet)
SKL_RFS_CLASS()

// 在编译期遍历已注册的成员
constexpr auto info = SRefl::type_info<Person>();
// info.fields, info.methods, 等等

// --- 动态反射 ---
SKL_RFD_CLASS(Person)
    SKL_RFD_PROPERTY(name)
    SKL_RFD_PROPERTY(age)
    SKL_RFD_METHOD(greet)
SKL_RFD_CLASS()

// 在运行时查询
auto *ti = DRefl::Registry::instance().find_by_name("Person");
auto *field = ti->find_field("name");
field->setter(&obj, &new_value);    // 类型擦除的字段写入
```

> [!NOTE]
> - [静态反射 API](docs/static_utils_api_zh.md)
> - [动态反射 API](docs/dynamic_api_zh.md)

## 注册宏

本库提供两级注册宏：

### 简化宏（推荐使用）

简洁统一的日常入口：

| 宏 | 说明 |
|---|------|
| `SKL_RFS_CLASS(ClassName)` | 开始静态类注册 |
| `SKL_RFS_PROPERTY(member)` | 注册静态字段或方法 |
| `SKL_RFS_CLASS()` | 结束静态类注册 |
| `SKL_RFS_ENUM(EnumName)` | 开始静态枚举注册 |
| `SKL_RFS_ENUM_VALUE(value, name)` | 注册静态枚举值 |
| `SKL_RFS_ENUM()` | 结束静态枚举注册 |
| `SKL_RFD_CLASS(ClassName)` | 开始动态类注册 |
| `SKL_RFD_PROPERTY(member)` | 注册动态属性 |
| `SKL_RFD_METHOD(method)` | 注册动态方法 |
| `SKL_RFD_CLASS()` | 结束动态类注册 |
| `SKL_RFD_ENUM(EnumName)` | 开始动态枚举注册 |
| `SKL_RFD_ENUM_VALUE(value, name)` | 注册动态枚举值 |
| `SKL_RFD_ENUM()` | 结束动态枚举注册 |

### 旧版宏（向后兼容）

原有的详细命名宏作为内部实现保留：

| 简化宏 | 旧版等价宏 |
|--------|-----------|
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

## 元数据系统（`metadata.h`）

元数据系统通过固定布局结构和外部字符串表提供稳定的 SKL_ABIX。

### 元数据模式

| 模式 | 值 | 说明 |
|------|---|------|
| `SKL_REFT_MODE_HASH` | `0` | 仅保存 hash，无字符串 — Release / 最小体积部署 |
| `SKL_REFT_MODE_STR` | `1` | 保存必要字符串 — Debug / 开发调试 |
| `SKL_REFT_MODE_FULL` | `2` | 保存完整调试元数据 — 编辑器 / IDE 集成 |

默认策略：Debug → `STR`，Release → `HASH`。可通过 `-DSKL_REFT_META_MODE=0|1|2` 手动覆盖。

### 核心类型

```cpp
// 固定 SKL_ABIX 结构 — 布局永远不变
struct MetaEntry {
    uint64_t id;              // FNV1a64 哈希
    uint32_t flags;           // MetaFlags 位掩码
    uint32_t metadataIndex;   // 索引到外部字符串表
};

// 外部字符串表 — 可跨 DLL 边界安全传输
struct MetaStringTable {
    uint32_t count;
    uint32_t dataSize;
    const StringEntry* entries;
    const char* data;
};
```

### constexpr 哈希

```cpp
constexpr uint64_t id = SKL_REFT_HASH("Player.Health");   // FNV1a64，跨编译器稳定
constexpr uint32_t id32 = SKL_REFT_HASH32("Player.Health");
```

## 目录结构

```
MICS/
├── mics.h                # 主入口头文件（包含 ct + rt）
├── metadata.h            # 元数据系统（MetaEntry、MetaMode、SKL_REFT_HASH、StringTable）
├── ct/                   # 编译期静态反射（mics::ct / SRefl）
│   ├── reflect.h         # 公共 API 与 field_traits、TypeInfo
│   ├── base_reflect.h    # __base_field_traits（变量/函数分发）
│   ├── base_fp.h         # 底层类型列表操作
│   ├── fp.h              # 公共类型列表 API（mics::ct::fp）
│   ├── var_traits.h      # 变量/字段萃取
│   ├── fn_traits.h       # 函数/方法萃取（修饰符、哈希）
│   ├── enum_traits.h     # 枚举萃取与有作用域枚举检测
│   ├── config.h          # type_list、template_depth、is_virtual_base_of
│   └── template_string.h # 非类型模板参数字符串支持
├── rt/                   # 运行时动态反射（mics::rt / DRefl）
│   ├── reflect.h         # 公共 API、注册宏、type_id_of
│   ├── config.h          # TypeId、Kind、FieldKind、Visibility 枚举
│   ├── type_info.h       # TypeInfo：聚合描述符（字段/方法/基类）
│   ├── field_info.h      # FieldInfo + FieldAccessor（getter/setter）
│   ├── fn_info.h         # FnInfo + MethodInvoker + ParamInfo
│   ├── enum_info.h       # EnumInfo + EnumEntry
│   ├── any.h             # Any：类型擦除值容器（SBO 优化）
│   └── registry.h        # Registry：全局单例类型注册表
├── util/                 # 共享工具（mics::util / URefl）
│   ├── hash.h            # FNV-1a 哈希函数、调用约定标签
│   ├── type_hash.h       # 编译期类型 → 哈希映射
│   ├── fn_hash.h         # 函数签名折叠哈希（compute_fn_hash）
│   ├── string_view.h     # 轻量级 string_view 实现
│   └── vector.h          # ABI 稳定不可变容器（动态反射数据载体）
├── doc/                  # API 文档
└── HLMD/                 # 宏工具库（内部）
```

## 支持的平台与工具链

| 平台 | 编译器 | 最低版本 | 状态 |
|----------|----------|-----------------|--------|
| Windows  | MSVC     | VS 2022 (17.0+) | ✓ |
| Windows  | MinGW-w64 (GCC) | 13.0+ | ✓ |
| Windows  | Clang-cl | 17.0+ | ✓ |
| Linux    | GCC      | 13.0+ | ✓ |
| Linux    | Clang    | 17.0+ | ✓ |

**要求：** C++17 或更高版本（推荐 C++20 以获得 `consteval` 和 `__cpp_nontype_template_args` 支持；推荐支持 concepts）

## 测试

测试使用 [Catch2](https://github.com/catchorg/Catch2)，入口文件 `main.cpp` 覆盖以下场景：

| 测试类别 | 标签 | 覆盖内容 |
|---------|------|----------|
| 静态类反射 | `[static][class]` | `Player`、`Weapon` 的类型元数据、字段读写、方法调用（含自定义模板名称和 const 方法） |
| 静态枚举反射 | `[static][enum]` | `Fruit`（无作用域枚举）、`Direction`（有作用域 `enum class`）的值列表与 `is_scoped` 检测 |
| 动态类反射 | `[dynamic][class]` | 通过 `Registry` 按名称查找类型、`FieldAccessor` 的 getter/setter、`FnInfo` 方法查找 |
| 动态枚举反射 | `[dynamic][enum]` | `EnumInfo` 的条目遍历、`is_scoped` 标志 |
| 注册表查询 | `[dynamic][registry]` | `find_by_name`、`type_count`、`type_at` 遍历 |
| 集成一致性 | `[integration]` | 验证静态反射与动态反射对同一类型的字段/方法数量和名称一致 |

```bash
# 构建并运行测试
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## 未来计划

- **MOC（元对象编译器）** — 计划基于 Clang LibTooling 实现，通过 C++ 属性（`[[...]]`）注解类与成员，自动生成静态和动态反射注册代码。开发者只需在类或字段上添加 `[[meta::export]]`、`[[meta::property]]`、`[[meta::method]]` 等属性，MOC 工具将扫描源文件并生成对应的注册代码，无需手动编写宏。

## 许可证

Apache License, Version 2.0。详见 [LICENSE](./LICENSE)。

---

Copyright 2026 [Sukanle](https://github.com/Sukanle)