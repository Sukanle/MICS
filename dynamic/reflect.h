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
#ifndef SKL_RELECT_DYNAMIC_H_
#define SKL_RELECT_DYNAMIC_H_

#include <stddef.h>
#include <stdint.h>

#include "utils/type_hash.h"   // IWYU pragma: keep

#include "dynamic/any.h"          // IWYU pragma: keep
#include "dynamic/config.h"       // IWYU pragma: keep
#include "dynamic/registry.h"     // IWYU pragma: keep
#include "dynamic/fn_info.h"      // IWYU pragma: keep
#include "dynamic/enum_info.h"    // IWYU pragma: keep
#include "dynamic/type_info.h"    // IWYU pragma: keep
#include "dynamic/field_info.h"   // IWYU pragma: keep

namespace Reflect::Dynamic {

template<typename T>
constexpr TypeId type_id_of() noexcept {
    return Utils::type_hash<T>();
}

namespace detail {

template<typename T>
struct member_type;

template<typename Class, typename T>
struct member_type<T Class::*> {
    using type = T;
    using class_type = Class;
};

template<typename Class, typename T>
struct member_type<T Class::* const> {
    using type = T;
    using class_type = Class;
};

template<auto mp>
FieldGetter make_field_getter() noexcept {
    using Class = typename member_type<decltype(mp)>::class_type;
    return [](void *obj) -> void * { return &(static_cast<Class *>(obj)->*mp); };
}

template<auto mp>
FieldSetter make_field_setter() noexcept {
    using Class = typename member_type<decltype(mp)>::class_type;
    using Member = typename member_type<decltype(mp)>::type;
    if constexpr (std::is_array_v<Member>) {
        return [](void *obj, void *value) {
            auto &arr = static_cast<Class *>(obj)->*mp;
            memcpy(&arr, value, sizeof(Member));
        };
    } else {
        return [](void *obj, void *value) { (static_cast<Class *>(obj)->*mp) = *static_cast<Member *>(value); };
    }
}

}   // namespace detail
}   // namespace Reflect::Dynamic
#endif