# 动态反射 API 参考

本文档涵盖运行时动态反射系统（`dynamic/`）。

---

## 架构概览

```
┌─────────────────────────────────────────────────────────┐
│  reflect.h（入口）                                       │
│  ┌───────────────────────────────────────────────────┐  │
│  │ 注册宏（简化版）                                    │  │
│  │ SKL_RFD_CLASS / SKL_RFD_PROPERTY / SKL_RFD_METHOD / ...      │  │
│  │ （旧版：SKL_RFD_REGISTER_BEGIN / SKL_RFD_FIELD / ...）    │  │
│  └──────────────┬────────────────────────────────────┘  │
│                 │ 填充                                   │
│  ┌──────────────▼────────────────────────────────────┐  │
│  │ TypeInfo（描述符）                                  │  │
│  │  ├─ FieldAccessor[]  （FieldInfo + getter/setter） │  │
│  │  ├─ FnInfo[]         （方法元数据 + invoker）      │  │
│  │  ├─ BaseInfo[]       （基类信息）                  │  │
│  │  └─ EnumInfo*        （枚举值，如适用）            │  │
│  └──────────────┬────────────────────────────────────┘  │
│                 │ 注册到                                 │
│  ┌──────────────▼────────────────────────────────────┐  │
│  │ Registry（单例）                                   │  │
│  │  find_by_id() / find_by_name() / for_each()       │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Any（类型擦除值容器）                               │  │
│  │  SBO ≤ 16 字节，try_cast<T>()                      │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 1. `config.h` — 核心类型定义

**命名空间：** `Reflect::Dynamic`

### 类型别名

| 符号 | 类型 | 说明 |
|--------|------|-------------|
| `TypeId` | `uint64_t` | 唯一类型标识符（基于哈希） |
| `FieldId` | `uint32_t` | 字段标识符 |
| `MethodId` | `uint32_t` | 方法标识符 |
| `INVALID_TYPE_ID` | `constexpr TypeId` | 哨兵值 `0` |

### 枚举

**`Kind`** — 类型分类：
| 值 | 说明 |
|-------|-------------|
| `Class` | `class` 类型 |
| `Struct` | `struct` 类型 |
| `Enum` | 枚举类型 |
| `Primitive` | 内置基本类型 |
| `Pointer` | 指针类型 |
| `Reference` | 引用类型 |

**`FieldKind`** — 字段/方法分类：
| 值 | 说明 |
|-------|-------------|
| `MemberVar` | 非静态成员变量 |
| `StaticVar` | 静态成员变量 |
| `MemberFn` | 非静态成员函数 |
| `StaticFn` | 静态成员函数 |

**`Visibility`** — 访问级别：
| 值 | 说明 |
|-------|-------------|
| `Public` | 公开访问 |
| `Protected` | 受保护访问 |
| `Private` | 私有访问 |

---

## 2. `field_info.h` — 字段描述符与访问器

**命名空间：** `Reflect::Dynamic`

### `FieldInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `const char*` | 字段名称 |
| `type_id` | `TypeId` | 字段类型的哈希值 |
| `offset` | `uint32_t` | 在类中的字节偏移量 |
| `kind` | `FieldKind` | MemberVar / StaticVar / MemberFn / StaticFn |
| `visibility` | `Visibility` | Public / Protected / Private |

**方法：**
| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `is_function()` | `bool` | `kind == MemberFn \|\| kind == StaticFn` |
| `is_variable()` | `bool` | `kind == MemberVar \|\| kind == StaticVar` |
| `is_static()` | `bool` | `kind == StaticVar \|\| kind == StaticFn` |

### `FieldAccessor`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `info` | `FieldInfo` | 字段元数据 |
| `getter` | `FieldGetter` | `void*(*)(void* obj)` — 类型擦除的 getter |
| `setter` | `FieldSetter` | `void(*)(void* obj, void* value)` — 类型擦除的 setter |

**使用示例：**
```cpp
auto *field = type_info->find_field("age");
if (field && field->getter) {
    int *age = static_cast<int*>(field->getter(&obj));
    *age = 42;
    // 或者等效地：
    int newAge = 30;
    field->setter(&obj, &newAge);
}
```

---

## 3. `fn_info.h` — 方法描述符与调用器

**命名空间：** `Reflect::Dynamic`

### `ParamInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `const char*` | 参数名称 |
| `type_id` | `TypeId` | 参数类型的哈希值 |

### `FnInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `const char*` | 方法名称 |
| `return_type_id` | `TypeId` | 返回类型的哈希值 |
| `params` | `Utils::vector<ParamInfo>` | 参数列表（ABI 稳定、不可变容器） |
| `invoker` | `MethodInvoker` | `void(*)(void* obj, void** args, void* result)` |
| `visibility` | `Visibility` | 访问级别 |
| `is_const` | `bool` | 方法是否为 `const` 限定 |
| `is_static` | `bool` | 方法是否为 `static` |

### `MethodInvoker`

```cpp
using MethodInvoker = void (*)(void *obj, void **args, void *result);
```

- `obj`：指向对象实例的指针（静态方法忽略）
- `args`：`void*` 指针数组，每个指向一个参数值
- `result`：`void*` 指向调用者分配的返回值存储空间

**使用示例：**
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

## 4. `enum_info.h` — 枚举描述符

**命名空间：** `Reflect::Dynamic`

### `EnumEntry`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `Utils::string_view` | 枚举值名称 |
| `value` | `int64_t` | 枚举值（有符号 64 位整数） |

### `EnumInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `const char*` | 枚举类型名称 |
| `type_id` | `TypeId` | 枚举类型的哈希值 |
| `underlying_type_id` | `TypeId` | 底层整数类型的哈希值 |
| `entries` | `Utils::vector<EnumEntry>` | 所有枚举值条目（ABI 稳定、不可变容器） |
| `is_scoped` | `bool` | 是否为 `enum class`（有作用域） |

**方法：**
| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `find_name(value)` | `const char*` | 按值查找名称，未找到返回 `nullptr` |
| `find_value(name, out)` | `bool` | 按名称查找值，设置 `out` 并返回 `true`（若找到） |

**使用示例：**
```cpp
auto *ei = type_info->enum_info;
if (ei) {
    auto *name = ei->find_name(2);        // 对于 Permission::write 返回 "write"
    int64_t val;
    if (ei->find_value("read", val)) {    // val = 1
        // ...
    }
}
```

---

## 5. `type_info.h` — 类型描述符

**命名空间：** `Reflect::Dynamic`

### `BaseInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `type_id` | `TypeId` | 基类类型的哈希值 |
| `name` | `const char*` | 基类名称 |
| `offset` | `int32_t` | 基类子对象在派生类中的字节偏移量（0 = 首个基类） |

### `TypeInfo`

| 成员 | 类型 | 说明 |
|--------|------|-------------|
| `name` | `const char*` | 类型名称 |
| `type_id` | `TypeId` | 唯一哈希标识符 |
| `kind` | `Kind` | Class / Struct / Enum / Primitive / Pointer / Reference |
| `size` | `size_t` | `sizeof(T)` |
| `bases` | `Utils::vector<BaseInfo>` | 直接基类（ABI 稳定、不可变容器） |
| `fields` | `Utils::vector<FieldAccessor>` | 已注册的字段（ABI 稳定、不可变容器） |
| `methods` | `Utils::vector<FnInfo>` | 已注册的方法（ABI 稳定、不可变容器） |
| `enum_info` | `const EnumInfo*` | 指向枚举信息的指针（仅当 `kind == Enum` 时非空） |

**方法：**
| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `find_field(name)` | `const FieldAccessor*` | 按名称查找字段，未找到返回 `nullptr` |
| `find_method(name)` | `const FnInfo*` | 按名称查找方法，未找到返回 `nullptr` |
| `has_base(type_id)` | `bool` | 检查 `type_id` 是否为直接基类 |

**使用示例：**
```cpp
auto *ti = Registry::instance().find_by_name("Person");
if (ti) {
    for (auto &f : ti->fields) {
        printf("字段：%s（偏移量=%u）\n", f.info.name, f.info.offset);
    }
    for (auto &m : ti->methods) {
        printf("方法：%s（参数个数=%zu）\n", m.name, m.params.size());
    }
}
```

---

## 6. `any.h` — 类型擦除值容器

**命名空间：** `Reflect::Dynamic`

`Any` 是一个轻量级类型擦除值容器，类似于 `std::any`，针对反射场景进行了优化。

### 设计

- **SBO（小缓冲区优化）：** ≤ 16 字节的值在联合体缓冲区中内联存储，避免堆分配
- **较大对象：** 在堆上分配，使用基于函数指针的 `Handler`（clone/destroy）
- **类型安全：** `type_id()` 返回所包含类型的 `TypeId` 哈希值

### API

| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `Any()` | — | 默认构造函数，空值 |
| `Any(T&& value)` | — | 从任意可移动值构造 |
| `Any(const Any&)` | — | 拷贝构造函数（通过 clone handler 深拷贝） |
| `Any(Any&&)` | — | 移动构造函数（窃取缓冲区） |
| `operator=(Any)` | `Any&` | 拷贝并交换赋值 |
| `swap(Any&)` | `void` | 交换内容 |
| `type_id()` | `TypeId` | 所包含值的类型哈希 |
| `has_value()` | `bool` | 是否包含值 |
| `operator bool()` | `bool` | 等同于 `has_value()` |
| `raw_ptr()` | `void*` | 指向所包含值的原始指针（SBO 或堆） |
| `try_cast<T>()` | `T*` | 若类型匹配则转换为 `T*`，否则返回 `nullptr` |
| `try_cast<T>() const` | `const T*` | const 版本 |

**使用示例：**
```cpp
Any a = 42;                         // int，SBO 内联
Any b = std::string("hello");       // string，堆分配

int *p = a.try_cast<int>();         // 成功，*p == 42
float *q = a.try_cast<float>();     // 失败，q == nullptr

auto tid = a.type_id();             // type_hash<int>()
```

---

## 7. `registry.h` — 全局类型注册表

**命名空间：** `Reflect::Dynamic`

`Registry` 是一个单例，收集所有静态注册的 `TypeInfo` 实例。

### API

| 方法 | 返回值 | 说明 |
|--------|---------|-------------|
| `instance()` | `Registry&` | 获取单例实例 |
| `register_type(info)` | `void` | 注册一个 `TypeInfo*`（若 `type_id` 重复则无操作） |
| `register_callback(cb)` | `void` | 注册外部回调，立即调用以注入反射数据 |
| `invoke_callbacks()` | `void` | 触发所有已注册的回调（用于延迟注册场景） |
| `find_by_id(id)` | `const TypeInfo*` | 按 `TypeId` 哈希查找 |
| `find_by_name(name)` | `const TypeInfo*` | 按类型名称字符串查找 |
| `type_count()` | `size_t` | 已注册类型数量 |
| `type_at(index)` | `const TypeInfo*` | 按索引获取类型，越界返回 `nullptr` |
| `for_each(fn)` | `void` | 使用回调 `fn(const TypeInfo*)` 遍历所有已注册类型 |
| `clear()` | `void` | 移除所有已注册类型 |

**使用示例：**
```cpp
auto &reg = Registry::instance();

// 按名称查找
if (auto *ti = reg.find_by_name("Person")) {
    // ...
}

// 遍历所有
reg.for_each([](const TypeInfo *ti) {
    printf("已注册：%s（id=%llu）\n", ti->name, ti->type_id);
});
```

---

## 8. `reflect.h` — 注册宏与公共 API

**命名空间：** `Reflect::Dynamic`

### `type_id_of<T>()`

```cpp
template<typename T>
constexpr TypeId type_id_of() noexcept;
```

返回类型 `T` 的编译期 `TypeId` 哈希值。委托给 `Utils::type_hash<T>()`。

### 注册宏

#### 简化宏（推荐使用）

```cpp
SKL_RFD_CLASS(ClassName)
    SKL_RFD_PROPERTY(member)                       // 注册成员变量
    SKL_RFD_PROPERTY(member, SKL_REFT_META_STRING)     // 强制保存字符串
    SKL_RFD_METHOD(method)                         // 注册成员函数
    SKL_RFD_METHOD(method, SKL_REFT_META_STRING)       // 强制保存字符串
SKL_RFD_CLASS_END()
```

| 宏 | 说明 |
|---|------|
| `SKL_RFD_CLASS(ClassName)` | 开始类注册，创建 `TypeInfo` |
| `SKL_RFD_PROPERTY(member)` | 注册非静态成员变量（自动生成 getter/setter） |
| `SKL_RFD_PROPERTY(member, SKL_REFT_META_STRING)` | 注册并强制保存字符串（覆盖 `SKL_REFT_META_MODE`） |
| `SKL_RFD_METHOD(method)` | 注册成员函数（记录名称、签名和调用器） |
| `SKL_RFD_METHOD(method, SKL_REFT_META_STRING)` | 注册并强制保存字符串 |
| `SKL_RFD_CLASS_END()` | 结束类注册，注册到 `Registry` |

#### 旧版宏（向后兼容）

```cpp
SKL_RFD_REGISTER_BEGIN(ClassName)
    SKL_RFD_FIELD(member)          // 注册成员变量
    SKL_RFD_METHOD(method)         // 注册成员函数
SKL_RFD_CLASS_END()                // 或 SKL_RFD_REGISTER_END()
```

| 简化宏 | 旧版等价宏 |
|--------|-----------|
| `SKL_RFD_CLASS(T)` | `SKL_RFD_REGISTER_BEGIN(T)` |
| `SKL_RFD_PROPERTY(m)` | `SKL_RFD_FIELD(m)` |
| `SKL_RFD_CLASS_END()` | `SKL_RFD_REGISTER_END()` |

#### 枚举注册

```cpp
SKL_RFD_ENUM(EnumName)
    SKL_RFD_ENUM_VALUE(value, "Name1")
    SKL_RFD_ENUM_VALUE(value, "Name2")
SKL_RFD_ENUM_END()
```

| 宏 | 说明 |
|-------|-------------|
| `SKL_RFD_ENUM(EnumName)` | 开始枚举注册，创建 `EnumInfo` 和 `TypeInfo` |
| `SKL_RFD_ENUM_VALUE(value, name)` | 注册一个枚举值条目 |
| `SKL_RFD_ENUM_END()` | 结束枚举注册，完成注册 |

> **注意：** 所有注册宏内部使用 `Utils::vector_builder` 构建数据，注册完成后数据转为不可变的 `Utils::vector`，确保 ABI 稳定。

### 内部辅助函数（`detail` 命名空间）

| 函数 | 说明 |
|----------|-------------|
| `member_type<T Class::*>` | 从成员指针提取 `type` 和 `class_type` |
| `make_field_getter<auto mp>()` | 从成员指针（模板参数）创建 `FieldGetter` |
| `make_field_setter<auto mp>()` | 从成员指针（模板参数）创建 `FieldSetter` |

---

## 9. 完整使用示例

```cpp
#include "reflect.h"

// ============================================================
// 定义一个类
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
// 使用动态反射注册
// ============================================================
SKL_RFD_CLASS(Player)
    SKL_RFD_PROPERTY(name)
    SKL_RFD_PROPERTY(level)
    SKL_RFD_PROPERTY(health)
    SKL_RFD_METHOD(takeDamage)
    SKL_RFD_METHOD(isAlive)
SKL_RFD_CLASS_END()

// ============================================================
// 注册一个枚举
// ============================================================
enum class ItemRarity { Common, Rare, Epic, Legendary };

SKL_RFD_ENUM(ItemRarity)
    SKL_RFD_ENUM_VALUE(Common, "Common")
    SKL_RFD_ENUM_VALUE(Rare, "Rare")
    SKL_RFD_ENUM_VALUE(Epic, "Epic")
    SKL_RFD_ENUM_VALUE(Legendary, "Legendary")
SKL_RFD_ENUM_END()

// ============================================================
// 运行时使用
// ============================================================
void runtime_example() {
    using namespace Reflect::Dynamic;

    // --- 查询类型信息 ---
    auto *ti = Registry::instance().find_by_name("Player");
    if (!ti) return;

    // --- 通过反射创建和操作对象 ---
    Player p;

    // 读取字段
    auto *nameField = ti->find_field("name");
    if (nameField && nameField->getter) {
        auto *namePtr = static_cast<std::string*>(nameField->getter(&p));
        *namePtr = "Hero";
    }

    // 写入字段
    auto *hpField = ti->find_field("health");
    if (hpField && hpField->setter) {
        float newHp = 75.0f;
        hpField->setter(&p, &newHp);
    }

    // 调用方法
    auto *method = ti->find_method("takeDamage");
    if (method && method->invoker) {
        float dmg = 25.0f;
        void *args[] = { &dmg };
        method->invoker(&p, args, nullptr);
    }

    // --- 枚举查找 ---
    auto *eTi = Registry::instance().find_by_name("ItemRarity");
    if (eTi && eTi->enum_info) {
        auto *name = eTi->enum_info->find_name(2);  // "Epic"
        int64_t val;
        eTi->enum_info->find_value("Legendary", val); // val = 3
    }
}
```

---

## 10. 设计要点

1. **静态初始化：** 注册通过全局对象构造函数在 `main()` 之前完成。`static bool` 守卫防止重复注册。
2. **类型 ID 一致性：** `TypeId` 使用 `Utils::type_hash<T>()`，确保静态和动态反射使用相同的哈希值。
3. **字段访问器设计：** `FieldGetter`/`FieldSetter` 使用转换为函数指针的非捕获 lambda。成员指针作为模板参数（`template<auto mp>`）传递以实现此功能。
4. **线程安全：** `Registry` 在并发注册时不是线程安全的。所有注册应在静态初始化期间（单线程）完成。
5. **SBO 阈值：** `Any` 类使用 16 字节的内联缓冲区。大于 16 字节或非平凡可拷贝的类型将进行堆分配。
6. **ABI 稳定容器：** 所有反射数据（`bases`、`fields`、`methods`、`params`、`entries`）使用 `Utils::vector<T>` 存储。该容器为不可变、仅移动类型，内部使用 `malloc`/`free` 管理内存，`sizeof` 固定为 24 字节（64 位），确保跨版本 ABI 兼容。若需外部操作，调用 `.to_std()` 转换为 `std::vector`。
7. **注册回调接口：** 外部模块可通过 `Registry::register_callback()` 注入反射数据。回调接收 `Registry&` 引用，可在其中调用 `register_type()` 注册类型。系统在回调返回后安全接管数据所有权。