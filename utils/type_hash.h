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
#ifndef SKL_UTILS_TYPE_HASH_H
#define SKL_UTILS_TYPE_HASH_H

#include <stdint.h>
#include <type_traits>

#include "utils/hash.h"   // IWYU pragma: keep

namespace Reflect::Utils {

template<typename T>
struct type_tag {
    static constexpr bool defined = false;
};

#define STATIC_TYPE_TAG(T, tag)                                                          \
    template<>                                                                           \
    struct Reflect::Utils::type_tag<T> {                                                 \
        static constexpr bool defined = true;                                            \
        static constexpr ::Reflect::Utils::hash64_t value = ::Reflect::Utils::cstr64(tag); \
    }

namespace detail {

template<typename T>
struct type_hash_impl;

#define STATIC_DEF_TYPE_HASH(T, tag)                   \
    template<>                                         \
    struct type_hash_impl<T> {                         \
        static constexpr hash64_t value = cstr64(tag); \
    }

STATIC_DEF_TYPE_HASH(void, "v");
STATIC_DEF_TYPE_HASH(bool, "b");
STATIC_DEF_TYPE_HASH(char, "c");
STATIC_DEF_TYPE_HASH(int8_t, "i8");
STATIC_DEF_TYPE_HASH(uint8_t, "u8");
STATIC_DEF_TYPE_HASH(int16_t, "i16");
STATIC_DEF_TYPE_HASH(uint16_t, "u16");
STATIC_DEF_TYPE_HASH(int32_t, "i32");
STATIC_DEF_TYPE_HASH(uint32_t, "u32");
STATIC_DEF_TYPE_HASH(long, "l");
STATIC_DEF_TYPE_HASH(unsigned long, "ul");
STATIC_DEF_TYPE_HASH(int64_t, "i64");
STATIC_DEF_TYPE_HASH(uint64_t, "u64");
STATIC_DEF_TYPE_HASH(float, "f");
STATIC_DEF_TYPE_HASH(double, "d");
STATIC_DEF_TYPE_HASH(long double, "ld");
#if __cpp_char8_t >= 201'811L
STATIC_DEF_TYPE_HASH(char8_t, "c8");
#endif
STATIC_DEF_TYPE_HASH(char16_t, "c16");
STATIC_DEF_TYPE_HASH(char32_t, "c32");
STATIC_DEF_TYPE_HASH(wchar_t, "wc");

#undef STATIC_DEF_TYPE_HASH

template<typename T>
struct type_hash_impl<T *> {
    static constexpr hash64_t value = mix(cstr64("p"), type_hash_impl<std::remove_cv_t<T>>::value);
};

template<typename T>
struct type_hash_impl<T &> {
    static constexpr hash64_t value = mix(cstr64("l"), type_hash_impl<std::remove_cv_t<T>>::value);
};

template<typename T>
struct type_hash_impl<T &&> {
    static constexpr hash64_t value = mix(cstr64("r"), type_hash_impl<std::remove_cv_t<T>>::value);
};

template<typename T>
struct dependent_false : std::false_type {};

template<typename T>
struct type_hash_impl {
    static constexpr hash64_t value = []() {
        if constexpr (type_tag<T>::defined) {
            return type_tag<T>::value;
        } else {
            static_assert(
                dependent_false<T>::value, "Type must be registered with STATIC_TYPE_TAG(T, \"name\") before use");
            return 0;
        }
    }();
};

}   // namespace detail

template<typename T>
constexpr hash64_t type_hash() noexcept {
    using U = std::conditional_t<std::is_reference_v<T>, T, std::remove_cv_t<T>>;
    return detail::type_hash_impl<U>::value;
}

template<typename T>
inline constexpr hash64_t type_hash_v = type_hash<T>();

}   // namespace Reflect::Utils

#endif