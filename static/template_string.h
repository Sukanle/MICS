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

#include <string_view>   // IWYU pragma: keep
#ifndef __cpp_consteval
#  define consteval constexpr
#endif

#if defined(__cpp_nontype_template_args) && __cpp_nontype_template_args >= 201'911L
#  include <algorithm>
#  define SKL_TEMPLATE_STRING_SUPPORTED 1
#else
#  define SKL_TEMPLATE_STRING_SUPPORTED 0
#endif

namespace Reflect::Static {
#define DEF2STR(macro_define) #macro_define
#define NUM2STR(num_define) DEF2STR(num_define)

#if SKL_TEMPLATE_STRING_SUPPORTED
#  define MAX_VARNAME_SIZE_CONIG 32
#  define MAX_VARNAME_SIZE (MAX_VARNAME_SIZE_CONIG)

template<uint8_t N>
struct template_string {
    static_assert(N <= MAX_VARNAME_SIZE,
        "Variable name exceeds maximum allowed length "
        "of " NUM2STR(MAX_VARNAME_SIZE_CONIG));
    consteval template_string(const char (&str)[N]) { std::copy(str, str + N, _data); }
    [[nodiscard]] consteval const char *data() const noexcept { return _data; }
    [[nodiscard]] consteval uint8_t size() const noexcept { return N - 1; }
    char _data[N];
};
#  define SKL_DEFAULT_TEMPLATE_STRING(name, str) \
      Reflect::Static::template_string name = Reflect::Static::template_string { str }
#  define SKL_NORMAL_TEMPLATE_STRING(name) Reflect::Static::template_string name
#  define SKL_MAKE_TEMPLATE_STRING(str) , str
template<template_string Name>
consteval auto NameAccessor() {
    return Name.data();
}
#else
#  define SKL_DEFAULT_TEMPLATE_STRING(name, str) const char *name = nullptr
#  define SKL_NORMAL_TEMPLATE_STRING(name) const char *name
#  define SKL_MAKE_TEMPLATE_STRING(str)
template<const char *Name>
consteval const char *NameAccessor() {
    return Name;
}
#endif
}   // namespace Reflect::Static
