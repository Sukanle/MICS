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
#ifndef SKL_UTILS_HASH_H
#define SKL_UTILS_HASH_H

#include <stdint.h>

namespace Reflect::Utils {

using hash64_t = uint64_t;
using hash32_t = uint32_t;

constexpr hash64_t FNV1A_OFFSET_BASE = 14'695'981'039'346'656'037ULL;
constexpr hash64_t FNV1A_PRIME = 1'099'511'628'211ULL;
constexpr hash64_t FNV1A_MASK = 0xFFFFFFFFULL;
constexpr hash64_t FNV1A_MIX = 0x9e3779b97f4a7c15ULL;

constexpr hash64_t fnv1a(const char *s, size_t n) noexcept {
    hash64_t h = FNV1A_OFFSET_BASE;
    for (size_t i = 0; i < n; ++i) {
        h ^= static_cast<uint8_t>(s[i]);
        h *= FNV1A_PRIME;
    }
    return h;
}

constexpr hash64_t cstr64(const char *s) noexcept {
    hash64_t h = FNV1A_OFFSET_BASE;
    for (; s && *s; ++s) {
        h ^= static_cast<uint8_t>(*s);
        h *= FNV1A_PRIME;
    }
    return h;
}

constexpr hash32_t cstr32(const char *s) noexcept { return static_cast<hash32_t>(cstr64(s) & FNV1A_MASK); }

constexpr hash64_t mix(hash64_t a, hash64_t b) noexcept { return a ^ (b + FNV1A_MIX + (a << 6) + (a >> 2)); }

constexpr hash64_t version(const char *v) noexcept { return cstr64(v); }

namespace cc {
enum class tag : uint8_t {
    Cdecl = 0,
    Stdcall = 1,
    Fastcall = 2,
    Vectorcall = 3
};
}   // namespace cc

}   // namespace Reflect::Utils

#endif