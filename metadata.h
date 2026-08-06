/*
 * Copyright 2026 Sukanle(https://github.com/Sukanle)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*===---- metadata.h - ABI Metadata System -------------===*\
 *
 * @file
 * @brief Fixed ABI metadata structures and metadata mode control.
 * @author Sukanle(https://github.com/Sukanle)
 * @date 2026-08-06
 * @version v0_1_0
 * @license MIT
 *
 * 核心设计原则：
 * 1. MetaEntry 结构体布局固定，不因 Debug/Release 改变 sizeof
 * 2. 字符串数据通过 StringTable 外部存储，不嵌入核心结构
 * 3. 三层 Metadata 模式：HASH(Release) / STR(Debug) / FULL(IDE)
 * 4. 用户覆盖优先级：单字段 > 编译参数 > Debug/Release 默认
 *
\*===----------------------------------------------------------------------===*/

#ifndef SKL_REFLECT_METADATA_H
#define SKL_REFLECT_METADATA_H

#include <stdint.h>

namespace Reflect {

#define SKL_REFT_MODE_HASH 0
#define SKL_REFT_MODE_STR 1
#define SKL_REFT_MODE_FULL 2

#ifndef SKL_REFT_META_MODE
#  ifdef NDEBUG
#    define SKL_REFT_META_MODE SKL_REFT_MODE_HASH
#  else
#    define SKL_REFT_META_MODE SKL_REFT_MODE_STR
#  endif
#endif

enum class MetaMode : uint8_t {
    HASH = SKL_REFT_MODE_HASH,
    STR = SKL_REFT_MODE_STR,
    FULL = SKL_REFT_MODE_FULL
};

// ================================================================
// 条件编译辅助宏
// ================================================================

#if SKL_REFT_META_MODE >= SKL_REFT_MODE_STR
#  define SKL_REFT_HAS_STRINGS 1
#else
#  define SKL_REFT_HAS_STRINGS 0
#endif

#if SKL_REFT_META_MODE >= SKL_REFT_MODE_FULL
#  define SKL_REFT_HAS_FULL_META 1
#else
#  define SKL_REFT_HAS_FULL_META 0
#endif

// 获取当前编译模式的 MetaMode 常量
constexpr MetaMode current_meta_mode() noexcept { return static_cast<MetaMode>(SKL_REFT_META_MODE); }

// ================================================================
// 固定 ABI 结构 —— MetaEntry
// ================================================================
// 此结构体布局在任何编译模式下都保持不变。
// 严禁使用 #ifdef 改变其成员或大小。
// 字符串数据通过 metadataIndex 索引外部 StringTable 获取。

struct MetaEntry {
    uint64_t id;              // 稳定 hash（FNV1a64），跨编译器一致
    uint32_t flags;           // 位标志，见 MetaFlags
    uint32_t metadataIndex;   // 索引到 StringTable / metadata section
};

// MetaEntry 标志位定义
enum MetaFlags : uint32_t {
    META_FLAG_NONE = 0,

    // 数据可用性标志
    META_FLAG_HAS_NAME = 1u << 0,        // 名称字符串可用（STR/FULL 模式）
    META_FLAG_HAS_TYPE = 1u << 1,        // 类型字符串可用
    META_FLAG_HAS_SIGNATURE = 1u << 2,   // 签名字符串可用（FULL 模式）
    META_FLAG_HAS_COMMENT = 1u << 3,     // 注释/文档字符串可用（FULL 模式）

    // 实体类型标志
    META_FLAG_IS_CLASS = 1u << 4,
    META_FLAG_IS_STRUCT = 1u << 5,
    META_FLAG_IS_ENUM = 1u << 6,
    META_FLAG_IS_METHOD = 1u << 7,
    META_FLAG_IS_PROPERTY = 1u << 8,
    META_FLAG_IS_BASE = 1u << 9,

    // 修饰符标志
    META_FLAG_IS_STATIC = 1u << 10,
    META_FLAG_IS_CONST = 1u << 11,
    META_FLAG_IS_VIRTUAL = 1u << 12,
    META_FLAG_IS_PUBLIC = 1u << 13,
    META_FLAG_IS_PROTECTED = 1u << 14,
    META_FLAG_IS_PRIVATE = 1u << 15,

    // 预留：16-31 供未来扩展
    META_FLAG_RESERVED_MASK = 0xFFFF0000u
};

// ================================================================
// 外部字符串表 —— StringTable
// ================================================================
// 不与 MetaEntry 耦合，独立存储，可跨 DLL 边界安全传输。

struct StringEntry {
    uint64_t hash;     // 字符串 hash，用于校验
    uint32_t offset;   // 在 data 段中的字节偏移
    uint32_t length;   // 字符串长度（不含 '\0'）
};

struct MetaStringTable {
    uint32_t count;               // 条目数
    uint32_t dataSize;            // 数据段总字节数（含所有 '\0'）
    uint32_t reserved;            // 对齐保留
    const StringEntry *entries;   // 条目数组
    const char *data;             // 拼接的字符串数据（以 '\0' 分隔）
};

// 通过索引查找字符串
inline const char *meta_string_lookup(const MetaStringTable *table, uint32_t index) noexcept {
    if (!table || index >= table->count) return nullptr;
    return table->data + table->entries[index].offset;
}

// 通过 hash 查找字符串（线性扫描，适用于小表）
inline const char *meta_string_lookup_by_hash(const MetaStringTable *table, uint64_t hash) noexcept {
    if (!table) return nullptr;
    for (uint32_t i = 0; i < table->count; ++i) {
        if (table->entries[i].hash == hash) {
            return table->data + table->entries[i].offset;
        }
    }
    return nullptr;
}

#define SKL_REFT_HASH(str) ::Reflect::Utils::hash_cstr(str)
#define SKL_REFT_HASH32(str) ::Reflect::Utils::hash_cstr32(str)

#define SKL_REFT_META_STRING 1

constexpr bool meta_should_store_string(int mode_override) noexcept {
    if (mode_override) return true;
    return SKL_REFT_META_MODE >= SKL_REFT_MODE_STR;
}

constexpr bool meta_should_store_full(int mode_override) noexcept {
    if (mode_override) return true;
    return SKL_REFT_META_MODE >= SKL_REFT_MODE_FULL;
}

constexpr const char *meta_string_or_null(const char *str, int mode_override) noexcept {
    return meta_should_store_string(mode_override) ? str : nullptr;
}

}   // namespace Reflect
#endif   // SKL_REFLECT_METADATA_H
