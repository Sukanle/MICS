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
#ifndef SKL_MICS_RT_TYPE_INFO_H
#define SKL_MICS_RT_TYPE_INFO_H

#include <stdint.h>

#include "utils/string_view.h"   // IWYU pragma: keep
#include "utils/vector.h"        // IWYU pragma: keep

#include "rt/config.h"       // IWYU pragma: keep
#include "rt/fn_info.h"      // IWYU pragma: keep
#include "rt/enum_info.h"    // IWYU pragma: keep
#include "rt/field_info.h"   // IWYU pragma: keep

namespace mics::rt {

struct BaseInfo {
    TypeId type_id;
    const char *name;
    int32_t offset;
};

struct TypeInfo {
    const char *name;
    TypeId type_id;
    Kind kind;
    size_t size;
    mics::utils::vector<BaseInfo> bases;
    mics::utils::vector<FieldAccessor> fields;
    mics::utils::vector<FnInfo> methods;
    const EnumInfo *enum_info;

    TypeInfo() noexcept
        : name(nullptr)
        , type_id(INVALID_TYPE_ID)
        , kind(Kind::Struct)
        , size(0)
        , bases(mics::utils::vector_empty)
        , fields(mics::utils::vector_empty)
        , methods(mics::utils::vector_empty)
        , enum_info(nullptr) {}

    const FieldAccessor *find_field(const char *field_name) const noexcept {
        for (auto &f : fields) {
            if (f.info.name && mics::utils::string_view(f.info.name) == field_name) return &f;
        }
        return nullptr;
    }

    const FnInfo *find_method(const char *method_name) const noexcept {
        for (auto &m : methods) {
            if (m.name && mics::utils::string_view(m.name) == method_name) return &m;
        }
        return nullptr;
    }

    bool has_base(TypeId base_id) const noexcept {
        for (auto &b : bases) {
            if (b.type_id == base_id) return true;
        }
        return false;
    }
};

}   // namespace mics::rt
#endif