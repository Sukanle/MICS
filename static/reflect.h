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
#ifndef SKL_RELECT_STATIC_H_
#define SKL_RELECT_STATIC_H_

#include "utils/string_view.h"   // IWYU pragma: keep

#include "static/fp.h"             // IWYU pragma: keep
#include "static/base_reflect.h"   // IWYU pragma: keep

namespace Reflect::Static {

template<template<typename...> class F, typename... Args>
inline constexpr bool NOT = !F<Args...>::value;

template<class T, typename = std::enable_if_t<std::is_class_v<T> || std::is_enum_v<T>>>
struct TypeInfo;

template<typename T>
struct is_type_list : std::false_type {};
template<typename... Ts>
struct is_type_list<type_list<Ts...>> : std::true_type {};

template<typename T>
inline constexpr bool is_type_list_v = is_type_list<T>::value;

template<typename T>
struct cv_combinations {
    using type = type_list<T, std::add_const_t<T>, std::add_volatile_t<T>, std::add_cv_t<T>>;
};
template<typename T>
struct ref_combinations {
    using type = type_list<T, std::add_lvalue_reference_t<T>, std::add_rvalue_reference_t<T>>;
};

template<typename T>
using cv_combinations_t = typename cv_combinations<T>::type;
template<typename T>
using ref_combinations_t = typename ref_combinations<T>::type;

#if !SKL_TEMPLATE_STRING_SUPPORTED
constexpr Utils::string_view strip_prefix(Utils::string_view name, Utils::string_view prefix = "&class_t::") noexcept {
    return name.substr(prefix.size());
}
#endif

template<typename T, typename Class = void, SKL_DEFAULT_TEMPLATE_STRING(Name, "")>
struct field_traits : __base_field_traits<T, is_Kind<T>(), Class, Name> {
    using traits = __base_field_traits<T, is_Kind<T>(), Class, Name>;
    explicit field_traits() = default;
    [[nodiscard]] consteval Utils::string_view getName() const { return _name; }
    explicit field_traits(T &ptr, Utils::string_view name = "")
        : __base_field_traits<T, is_Kind<T>(), Class, Name>{ptr}
        , _name(name) {}
    explicit consteval field_traits(T &&ptr, Utils::string_view name = "")
        : __base_field_traits<T, is_Kind<T>(), Class, Name>{std::move(ptr)}
        , _name(name) {}

    [[nodiscard]] static consteval Utils::string_view from_TempName() {
#if !SKL_TEMPLATE_STRING_SUPPORTED
        static_assert(Name != nullptr,
            "This function [SRef::field_traits::from_TempName] must be "
            "an object constructed when passing in the second template "
            "argument to `SRef::field_traits` in order to be called; \n"
            "To fix this bug, pass in the second template variable, or "
            "call `SRef::field_traits obj.getName()`.");
#endif
        return NameAccessor<Name>();
    }

private:
    Utils::string_view _name;
};

template<typename T>
consteval auto type_info() {
    return TypeInfo<T>{};
}
}   // namespace Reflect::Static
#endif
