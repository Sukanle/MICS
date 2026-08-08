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
#ifndef SKL_RELECT_STATIC_FN_TRAITS_H_
#define SKL_RELECT_STATIC_FN_TRAITS_H_

#include <stdint.h>

#include "HLMD/HLMD.h"   // IWYU pragma: keep

#include "utils/hash.h"        // IWYU pragma: keep
#include "utils/type_hash.h"   // IWYU pragma: keep
#include "utils/fn_hash.h"   // IWYU pragma: keep

#include "static/config.h"            // IWYU pragma: keep
#include "static/template_string.h"   // IWYU pragma: keep

namespace Reflect::Static {

template<typename, typename = void>
struct fn_type;

template<typename Ret, typename... Args, typename Class>
struct fn_type<Ret(Args...), Class> {
    using ret_t = Ret;
    using class_t = Class;
    using args_t = std::tuple<Args...>;
};
#define SREFL_FNT_HELP(fn, ...)                                                                      \
    HLMD__VA_OPT__((decltype(HLMD_GET_OTHER_ARGS(__VA_ARGS__) HLMD_GET_FIRST_ARGS(__VA_ARGS__)::fn), \
                       HLMD_GET_FIRST_ARGS(__VA_ARGS__)),                                            \
        (decltype(fn)), HLMD_GET_FIRST_ARGS(__VA_ARGS__))
template<typename Ret, typename... Args, typename Class>
struct fn_type<Ret (*)(Args...), Class> : fn_type<Ret(Args...), Class> {};
template<typename Ret, typename Class, typename... Args>
struct fn_type<Ret (Class::*)(Args...)> {
    using ret_t = Ret;
    using class_t = Class;
    using args_t = std::tuple<Args...>;
};

template<typename, typename = void, SKL_DEFAULT_TEMPLATE_STRING(, "")>
struct __base_fn_traits;

template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Class, typename Ret, typename... Args>
struct __base_fn_traits<Ret(Args...), Class, Name> : fn_type<Ret(Args...), Class> {
    static constexpr auto name = Name;
    static constexpr bool is_member = false;
    static constexpr template_depth params_count = sizeof...(Args);
    static constexpr Reflect::Utils::hash64_t hash = Reflect::Utils::compute_fn_hash<Reflect::Utils::type_hash_v<Ret>, Reflect::Utils::type_hash_v<Args>...>();
};
template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Class, typename Ret, typename... Args>
struct __base_fn_traits<Ret (*)(Args...), Class, Name> : __base_fn_traits<Ret(Args...)> {};
template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Ret, typename Class, typename... Args>
struct __base_fn_traits<Ret (Class::*)(Args...), Class, Name> : fn_type<Ret (Class::*)(Args...)> {
    static constexpr auto name = Name;
    static constexpr bool is_member = true;
    static constexpr bool is_static = false;
    static constexpr template_depth params_count = sizeof...(Args);
    static constexpr Reflect::Utils::hash64_t hash = Reflect::Utils::compute_fn_hash<Reflect::Utils::type_hash_v<Ret>, Reflect::Utils::type_hash_v<Args>...>();
};

template<typename, typename = void, SKL_DEFAULT_TEMPLATE_STRING(, "")>
struct fn_traits;

namespace fn_qualify {
inline constexpr uint8_t SREFL_NOTHING = 0x00;
inline constexpr uint8_t SREFL_NOEXCEPT = 0x01;
inline constexpr uint8_t SREFL_CONST = 0x02;
inline constexpr uint8_t SREFL_VOLATILE = 0x04;
inline constexpr uint8_t SREFL_CV = 0x06;
inline constexpr uint8_t SREFL_LVALUE = 0x08;
inline constexpr uint8_t SREFL_RVALUE = 0x10;
}   // namespace fn_qualify

// NOLINTBEGIN
#define DEF_NOMEM_FN_TRAITS(modifier, value)                                                             \
    template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Class, typename Ret, typename... Args>           \
    struct fn_traits<Ret(Args...) modifier, Class, Name> : __base_fn_traits<Ret(Args...), Class, Name> { \
        using fn_ptr = Ret (*)(Args...) modifier;                                                        \
        using fn_t = Ret(Args...) modifier;                                                              \
        using type = fn_t;                                                                               \
        static constexpr uint8_t modifie = value;                                                        \
    };                                                                                                   \
    template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Class, typename Ret, typename... Args>           \
    struct fn_traits<Ret (*)(Args...) modifier, Class, Name> : fn_traits<Ret(Args...), Class, Name> {};

DEF_NOMEM_FN_TRAITS(, 0x00)
DEF_NOMEM_FN_TRAITS(noexcept, 0x01)

#define DEF_MEM_FN_TRAITS(modifier, value)                                                     \
    template<SKL_NORMAL_TEMPLATE_STRING(Name), typename Ret, typename Class, typename... Args> \
    struct fn_traits<Ret (Class::*)(Args...) modifier, Class, Name>                            \
        : __base_fn_traits<Ret (Class::*)(Args...), Class, Name> {                             \
        using m_fn_ptr = Ret (Class::*)(Args...) modifier;                                     \
        using fn_t = m_fn_ptr;                                                                 \
        using type = fn_t;                                                                     \
        static constexpr uint8_t modifie = value;                                              \
    };

DEF_MEM_FN_TRAITS(, 0x00)
DEF_MEM_FN_TRAITS(const, 0x02)
DEF_MEM_FN_TRAITS(volatile, 0x04)
DEF_MEM_FN_TRAITS(const volatile, 0x06)
DEF_MEM_FN_TRAITS(&, 0x08)
DEF_MEM_FN_TRAITS(const &, 0x0A)
DEF_MEM_FN_TRAITS(volatile &, 0x0C)
DEF_MEM_FN_TRAITS(const volatile &, 0x0E)
DEF_MEM_FN_TRAITS(&&, 0x10)
DEF_MEM_FN_TRAITS(const &&, 0x12)
DEF_MEM_FN_TRAITS(volatile &&, 0x14)
DEF_MEM_FN_TRAITS(const volatile &&, 0x16)
DEF_MEM_FN_TRAITS(noexcept, 0x01)
DEF_MEM_FN_TRAITS(const noexcept, 0x03)
DEF_MEM_FN_TRAITS(volatile noexcept, 0x05)
DEF_MEM_FN_TRAITS(const volatile noexcept, 0x07)
DEF_MEM_FN_TRAITS(& noexcept, 0x09)
DEF_MEM_FN_TRAITS(const & noexcept, 0x0B)
DEF_MEM_FN_TRAITS(volatile & noexcept, 0x0D)
DEF_MEM_FN_TRAITS(const volatile & noexcept, 0x0F)
DEF_MEM_FN_TRAITS(&& noexcept, 0x11)
DEF_MEM_FN_TRAITS(const && noexcept, 0x13)
DEF_MEM_FN_TRAITS(volatile && noexcept, 0x15)
DEF_MEM_FN_TRAITS(const volatile && noexcept, 0x17)
// NOLINTEND

#undef DEF_NOMEM_FN_TRAITS
#undef DEF_MEM_FN_TRAITS
}   // namespace Reflect::Static
#endif
