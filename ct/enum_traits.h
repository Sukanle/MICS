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
#ifndef SKL_MICS_CT_ENUM_TRAITS_H_
#define SKL_MICS_CT_ENUM_TRAITS_H_

#include <type_traits>

#include "utils/string_view.h"   // IWYU pragma: keep

namespace mics::ct {
template<typename E>
struct is_scoped_enum
    : std::integral_constant<bool, !std::is_convertible_v<E, typename std::underlying_type_t<E>> && std::is_enum_v<E>> {
};
template<typename E>
inline constexpr bool is_scoped_enum_v = is_scoped_enum<E>::value;
template<typename E>
struct is_normal_enum
    : std::integral_constant<bool, std::is_convertible_v<E, typename std::underlying_type_t<E>> && std::is_enum_v<E>> {
};
template<typename E>
inline constexpr bool is_normal_enum_v = is_normal_enum<E>::value;

template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
struct enum_type {
    using type = E;
    using underlying_t = std::underlying_type_t<E>;
};

template<typename E>
struct enum_traits : enum_type<E, void> {
    struct Entry {
        E value;
        mics::util::string_view name;
    };
    static constexpr bool is_scoped = is_scoped_enum_v<E>;
    explicit constexpr enum_traits() = default;
};
}   // namespace mics::ct
#endif