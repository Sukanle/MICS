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
#ifndef SKL_MICS_UTILS_ARRAY_H
#define SKL_MICS_UTILS_ARRAY_H

#include <stddef.h>
#include <utility>

namespace skl::mics::util {

// A fixed-size, zero-allocation array usable in constant expressions.
//
// The deduction guide lets a braced list name its own type, so a caller can
// pass one inline where a pointer + count pair would otherwise force a named
// array:
//
//   array{type_a, type_b}   // array<model::TypeId, 2>
//
// This is deliberately a plain aggregate over a C array: it has no allocator,
// no bounds-checked growth and no dependency on the standard library.
//
// `make_iota<T, N, Start>` builds an aggregate with N consecutive values. The
// default start is 1; zero-based slot tables (such as InterfaceProxy's Slots)
// pass Start = 0 explicitly.
template<typename T, size_t Count = 0>
struct array {
    T values[Count == 0 ? 1 : Count];

    constexpr const T *data() const noexcept { return values; }
    constexpr T *data() noexcept { return values; }
    static constexpr size_t size() noexcept { return Count; }
    constexpr const T &operator[](size_t index) const noexcept { return values[index]; }
    constexpr T &operator[](size_t index) noexcept { return values[index]; }
    constexpr const T *begin() const noexcept { return values; }
    constexpr const T *end() const noexcept { return values + Count; }
    constexpr T *begin() noexcept { return values; }
    constexpr T *end() noexcept { return values + Count; }
    friend constexpr bool operator==(const array &lhs, const array &rhs) noexcept {
        for (size_t i = 0; i < Count; ++i)
            if (!(lhs[i] == rhs[i])) return false;
        return true;
    }
    friend constexpr bool operator!=(const array &lhs, const array &rhs) noexcept { return !(lhs == rhs); }
};

struct empty_element {
    friend constexpr bool operator==(empty_element a, empty_element b) noexcept {
        (void)a;
        (void)b;
        return true;
    }
    friend constexpr bool operator!=(empty_element a, empty_element b) noexcept {
        (void)a;
        (void)b;
        return false;
    }
};

template<typename T, typename... Rest>
array(T, Rest...) -> array<T, 1 + sizeof...(Rest)>;

array() -> array<empty_element, 0>;

template<typename T, size_t N, size_t Start = 1, size_t... Is>
constexpr T make_index_sequence(std::index_sequence<Is...>) {
    return T{(Start + Is)...};
}

template<typename T, size_t N, size_t Start = 1>
constexpr T make_iota() {
    return make_index_sequence<T, N, Start>(std::make_index_sequence<N>{});
}

}   // namespace skl::mics::util

#endif   // SKL_MICS_UTILS_ARRAY_H
