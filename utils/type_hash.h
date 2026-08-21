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
#include <limits.h>
#include <math.h>
#include <type_traits>

#include "utils/hash.h"   // IWYU pragma: keep

namespace Reflect::Utils {

template<typename T>
struct type_tag {
    static constexpr bool defined = false;
};

#define STATIC_TYPE_TAG(T, tag)                                                            \
    template<>                                                                             \
    struct Reflect::Utils::type_tag<T> {                                                   \
        static constexpr bool defined = true;                                              \
        static constexpr ::Reflect::Utils::hash64_t value = ::Reflect::Utils::cstr64(tag); \
    }

namespace detail {

template<typename T>
struct type_hash_impl;

// Integral types excluding bool (handled separately) and char (character type).
// This trait drives automatic hash generation for all integer types based on
// sizeof + signedness, avoiding platform-dependent alias issues (long, long long, etc.).
template<typename T>
struct is_integral : std::integral_constant<bool, std::is_integral_v<T>
                                                      && !std::is_same_v<std::remove_cv_t<T>, bool>
                                                      && !std::is_same_v<std::remove_cv_t<T>, char>> {};

template<typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;

template<typename T, typename = std::enable_if_t<is_integral_v<T>>>
struct integer_trait {
    struct integer_info {
        bool is_signed;
        uint8_t bits;

        constexpr const char *tag() const noexcept {
            if (is_signed) {
                if (bits == 8) return "i8";
                if (bits == 16) return "i16";
                if (bits == 32) return "i32";
                if (bits == 64) return "i64";
            } else {
                if (bits == 8) return "u8";
                if (bits == 16) return "u16";
                if (bits == 32) return "u32";
                if (bits == 64) return "u64";
            }
            return "??";
        }
    };

    static constexpr integer_info info{std::is_signed_v<T>, static_cast<uint8_t>(sizeof(T) * CHAR_BIT)};
};


#define STATIC_DEF_TYPE_HASH(T, tag)                   \
    template<>                                         \
    struct type_hash_impl<T> {                         \
        static constexpr hash64_t value = cstr64(tag); \
    }

// Basic types (not integral)
STATIC_DEF_TYPE_HASH(void, "v");
STATIC_DEF_TYPE_HASH(bool, "b");
STATIC_DEF_TYPE_HASH(char, "c");
STATIC_DEF_TYPE_HASH(float, "f");
STATIC_DEF_TYPE_HASH(double, "d");
STATIC_DEF_TYPE_HASH(long double, "ld");

// Character types
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
        if constexpr (is_integral_v<T>) {
            return cstr64(integer_trait<T>::info.tag());
        } else if constexpr (type_tag<T>::defined) {
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