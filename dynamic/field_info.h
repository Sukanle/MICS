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
#ifndef SKL_RELECT_DYNAMIC_FIELD_INFO_H
#define SKL_RELECT_DYNAMIC_FIELD_INFO_H

#include <stdint.h>

#include "dynamic/config.h"   // IWYU pragma: keep

namespace Reflect::Dynamic {

struct FieldInfo {
    const char *name;
    TypeId type_id;
    uint32_t offset;
    FieldKind kind;
    Visibility visibility;

    FieldInfo() noexcept
        : name(nullptr)
        , type_id(INVALID_TYPE_ID)
        , offset(0)
        , kind(FieldKind::MemberVar)
        , visibility(Visibility::Public) {}

    FieldInfo(const char *n, TypeId tid, uint32_t off, FieldKind k = FieldKind::MemberVar,
        Visibility vis = Visibility::Public) noexcept
        : name(n)
        , type_id(tid)
        , offset(off)
        , kind(k)
        , visibility(vis) {}

    bool is_function() const noexcept { return kind == FieldKind::MemberFn || kind == FieldKind::StaticFn; }
    bool is_variable() const noexcept { return kind == FieldKind::MemberVar || kind == FieldKind::StaticVar; }
    bool is_static() const noexcept { return kind == FieldKind::StaticVar || kind == FieldKind::StaticFn; }
};

// 字段访问器：类型擦除�?getter / setter
// 由注册宏自动生成，调用者无需关心内部实现
using FieldGetter = void *(*)(void *obj);
using FieldSetter = void (*)(void *obj, void *value);

struct FieldAccessor {
    FieldInfo info;
    FieldGetter getter;
    FieldSetter setter;

    FieldAccessor() noexcept
        : info()
        , getter(nullptr)
        , setter(nullptr) {}

    FieldAccessor(FieldInfo f, FieldGetter g, FieldSetter s) noexcept
        : info(f)
        , getter(g)
        , setter(s) {}
};

}   // namespace Reflect::Dynamic
#endif
