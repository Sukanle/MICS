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
#ifndef SKL_RELECT_STATIC_CONFIG_H_
#define SKL_RELECT_STATIC_CONFIG_H_

#include <stdint.h>

#include <type_traits>   // IWYU pragma: keep

namespace Reflect::Static {
#ifndef MAX_TEMPLATE_DEPTH
#  define MAX_TEMPLATE_DEPTH std::numeric_limits<template_depth>::max()
using template_depth = uint8_t;
#endif
#ifndef MAX_TEMPLATE_CONSTANTS
#  define MAX_TEMPLATE_CONSTANTS std::numeric_limits<template_constants>::max()
using template_constants = uint8_t;
#endif

#if __has_builtin(__builtin_is_virtual_base_of)

template<class Base, class Derived>
struct is_virtual_base_of : public std::bool_constant<__builtin_is_virtual_base_of(Base, Derived)> {};

#else
template<class T, class U>
constexpr bool __base_is_virtual_base_of(...) {
    return true;
}

template<class T, class U, typename std::void_t<decltype((U *)(std::declval<T *>()))> * = nullptr>
constexpr bool __base_is_virtual_base_of(int) {
    return false;
}
template<class T, class U>
struct is_virtual_base_of
    : public std::integral_constant<bool,
          std::is_base_of_v<T, U> && __base_is_virtual_base_of<T, U>(0) && !__base_is_virtual_base_of<U, T>(0)> {};
template<class Base, class Derived>
struct is_virtual_base_of<Base &, Derived> : public std::false_type {};
template<class Base, class Derived>
struct is_virtual_base_of<Base, Derived &> : public std::false_type {};
template<class Base, class Derived>
struct is_virtual_base_of<Base &, Derived &> : public std::false_type {};

#endif
template<class Base, class Derived>
inline constexpr bool is_virtual_base_of_v = is_virtual_base_of<Base, Derived>::value;


template<typename... Args>
struct type_list {
    static constexpr template_depth count = sizeof...(Args);
};

using empty_list = type_list<>;

template<typename T>
struct is_type_list_of : std::false_type {};
template<typename... Args>
struct is_type_list_of<type_list<Args...>> : std::true_type {};
template<typename T>
inline constexpr bool is_type_list_of_v = is_type_list_of<T>::value;

}   // namespace Reflect::Static
#endif
