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
#ifndef SKL_MICS_CT_BASE_REFLECT_H_
#define SKL_MICS_CT_BASE_REFLECT_H_

#include "ct/enum_traits.h"   // IWYU pragma: keep
#include "ct/fn_traits.h"     // IWYU pragma: keep
#include "ct/var_traits.h"    // IWYU pragma: keep

namespace mics::ct {
enum class Kind : uint8_t {
    FreeOrStatic_Var,
    FreeOrStatic_Fn,
    NonStaticMem_Var,
    NonStaticMem_Fn,
};
template<typename T>
consteval Kind is_Kind() {
    if constexpr (std::is_member_function_pointer_v<T>)
        return Kind::NonStaticMem_Fn;
    else if constexpr (std::is_member_object_pointer_v<T>)
        return Kind::NonStaticMem_Var;
    else
        return std::is_function_v<T> ? Kind::FreeOrStatic_Fn : Kind::FreeOrStatic_Var;
}

template<typename T, auto fn_var_t, typename Class = void, SKL_DEFAULT_TEMPLATE_STRING(, "")>
struct __base_field_traits;

template<typename T, typename Class, SKL_NORMAL_TEMPLATE_STRING(Name)>
struct __base_field_traits<T, Kind::FreeOrStatic_Var, Class, Name> : var_traits<T, Name> {
    using traits = var_traits<T, Name>;

    [[nodiscard]] consteval bool is_member() const { return traits::is_member; }
    [[nodiscard]] consteval bool is_function() const { return false; }
    [[nodiscard]] consteval bool is_variable() const { return true; }
    explicit __base_field_traits(typename traits::type &ptr)
        : _ptr(ptr) {}
    explicit __base_field_traits(typename traits::type &&ptr)
        : _ptr(std::move(ptr)) {}

    typename traits::type _ptr;
};

template<typename T, typename Class, SKL_NORMAL_TEMPLATE_STRING(Name)>
struct __base_field_traits<T, Kind::NonStaticMem_Var, Class, Name> : var_traits<T, Name> {
    using traits = var_traits<T, Name>;

    [[nodiscard]] static consteval bool is_member() { return true; }
    [[nodiscard]] static consteval bool is_function() { return false; }
    [[nodiscard]] static consteval bool is_variable() { return true; }
    [[nodiscard]] static consteval bool is_static() { return false; }
    explicit consteval __base_field_traits(typename traits::m_var_ptr &&ptr)
        : _ptr(std::move(ptr)) {}

    typename traits::m_var_ptr _ptr;
};

template<typename T, typename Class, SKL_NORMAL_TEMPLATE_STRING(Name)>
struct __base_field_traits<T, Kind::FreeOrStatic_Fn, Class, Name> : fn_traits<T, Class, Name> {
    using traits = fn_traits<T, Class, Name>;

    [[nodiscard]] consteval static bool is_member() { return traits::is_member; }
    [[nodiscard]] consteval static bool is_function() { return true; }
    [[nodiscard]] consteval static bool is_variable() { return false; }
    [[nodiscard]] consteval static template_depth params_count() { return traits::params_count; }
    explicit __base_field_traits(typename traits::fn_ptr &&ptr)
        : _ptr(std::move(ptr)) {}

    typename traits::fn_ptr _ptr;
};
template<typename T, typename Class, SKL_NORMAL_TEMPLATE_STRING(Name)>
struct __base_field_traits<T, Kind::NonStaticMem_Fn, Class, Name> : fn_traits<T, Class, Name> {
    using traits = fn_traits<T, Class, Name>;

    [[nodiscard]] consteval static bool is_member() { return traits::is_member; }
    [[nodiscard]] consteval static bool is_static() { return traits::is_static; }
    [[nodiscard]] consteval static bool is_const() { return (traits::modifie & fn_qualify::SREFL_CONST); }
    [[nodiscard]] consteval static bool is_volatile() { return (traits::modifie & fn_qualify::SREFL_VOLATILE); }
    [[nodiscard]] consteval static bool is_lvalue() { return (traits::modifie & fn_qualify::SREFL_LVALUE); }
    [[nodiscard]] consteval static bool is_rvalue() { return (traits::modifie & fn_qualify::SREFL_RVALUE); }
    [[nodiscard]] consteval static bool is_noexcept() { return (traits::modifie & fn_qualify::SREFL_NOEXCEPT); }
    [[nodiscard]] consteval static bool is_function() { return true; }
    [[nodiscard]] consteval static bool is_variable() { return false; }
    [[nodiscard]] consteval static template_depth params_count() { return traits::params_count; }
    explicit consteval __base_field_traits(typename traits::m_fn_ptr &&ptr)
        : _ptr(std::move(ptr)) {}

    typename traits::m_fn_ptr _ptr;
};
}   // namespace mics::ct
#endif