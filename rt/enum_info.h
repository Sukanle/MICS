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
#ifndef SKL_MICS_RT_ENUM_INFO_H
#define SKL_MICS_RT_ENUM_INFO_H

#include <stdint.h>

#include <utility>

#include "utils/string_view.h"   // IWYU pragma: keep
#include "utils/vector.h"        // IWYU pragma: keep

#include "rt/config.h"   // IWYU pragma: keep

namespace mics::rt {

struct EnumEntry {
    int64_t value;
    mics::utils::string_view name;
};

struct EnumInfo {
    const char *name;
    TypeId type_id;
    TypeId underlying_type_id;
    mics::utils::vector<EnumEntry> entries;
    bool is_scoped;

    EnumInfo() noexcept
        : name(nullptr)
        , type_id(INVALID_TYPE_ID)
        , underlying_type_id(INVALID_TYPE_ID)
        , entries(mics::utils::vector_empty)
        , is_scoped(false) {}

    EnumInfo(const char *n, TypeId tid, TypeId utid, mics::utils::vector<EnumEntry> e, bool scoped = false) noexcept
        : name(n)
        , type_id(tid)
        , underlying_type_id(utid)
        , entries(std::move(e))
        , is_scoped(scoped) {}

    const char *find_name(int64_t value) const noexcept {
        for (auto &entry : entries) {
            if (entry.value == value) return entry.name.data();
        }
        return nullptr;
    }

    bool find_value(const char *name, int64_t &out) const noexcept {
        for (auto &entry : entries) {
            if (entry.name == name) {
                out = entry.value;
                return true;
            }
        }
        return false;
    }
};

}   // namespace mics::rt
#endif