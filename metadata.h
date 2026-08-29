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

#ifndef SKL_MICS_METADATA_H
#define SKL_MICS_METADATA_H

#include <stdint.h>

namespace mics {

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

struct MetaEntry {
    uint64_t id;
    uint32_t flags;
    uint32_t metadataIndex;
};

enum MetaFlags : uint32_t {
    META_FLAG_NONE = 0X0000,

    META_FLAG_HAS_NAME = 0X0001,
    META_FLAG_HAS_TYPE = 0X0002,
    META_FLAG_HAS_SIGNATURE = 0X0004,
    META_FLAG_HAS_COMMENT = 0X0008,

    META_FLAG_IS_CLASS = 0X0010,
    META_FLAG_IS_STRUCT = 0X0020,
    META_FLAG_IS_ENUM = 0X0040,
    META_FLAG_IS_METHOD = 0X0080,
    META_FLAG_IS_PROPERTY = 0X0100,
    META_FLAG_IS_BASE = 0X0200,

    META_FLAG_IS_STATIC = 0X0400,
    META_FLAG_IS_CONST = 0X0800,
    META_FLAG_IS_VIRTUAL = 0X1000,
    META_FLAG_IS_PUBLIC = 0X2000,
    META_FLAG_IS_PROTECTED = 0X4000,
    META_FLAG_IS_PRIVATE = 0X8000,

    META_FLAG_RESERVED_MASK = 0xFFFF0000
};


struct StringEntry {
    uint64_t hash;
    uint32_t offset;
    uint32_t length;
};

struct MetaStringTable {
    uint32_t count;
    uint32_t dataSize;
    uint32_t reserved;
    const StringEntry *entries;
    const char *data;
};

inline const char *meta_string_lookup(const MetaStringTable *table, uint32_t index) noexcept {
    if (!table || index >= table->count) return nullptr;
    return table->data + table->entries[index].offset;
}

inline const char *meta_string_lookup_by_hash(const MetaStringTable *table, uint64_t hash) noexcept {
    if (!table) return nullptr;
    for (uint32_t i = 0; i < table->count; ++i) {
        if (table->entries[i].hash == hash) {
            return table->data + table->entries[i].offset;
        }
    }
    return nullptr;
}

#define SKL_REFT_HASH(str) ::mics::util::hash_cstr(str)
#define SKL_REFT_HASH32(str) ::mics::util::hash_cstr32(str)

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

}   // namespace mics
#endif   // SKL_MICS_METADATA_H