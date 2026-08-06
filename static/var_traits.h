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
#pragma once

#include "static/template_string.h"   // IWYU pragma: keep

namespace Reflect::Static {

template<typename T>
struct var_type {
    using type = T;
    using var_ptr = T *;
};
template<typename Class, typename T>
struct var_type<T Class::*> {
    using m_var_ptr = T Class::*;
    using class_t = Class;
    using type = T;
};

template<typename T, SKL_DEFAULT_TEMPLATE_STRING(Name, "")>
struct __base_var_traits : var_type<T> {
    static constexpr auto name = Name;
    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
    static constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
};

template<typename T, SKL_DEFAULT_TEMPLATE_STRING(Name, "")>
struct var_traits : __base_var_traits<T, Name> {
    static constexpr bool is_member = false;
};

template<typename T, typename Class, SKL_NORMAL_TEMPLATE_STRING(Name)>
struct var_traits<T Class::*, Name> : __base_var_traits<T Class::*, Name> {
    static constexpr bool is_member = false;
};

}   // namespace Reflect::Static
