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
#ifndef SKL_MICS_RT_REGISTRY_H
#define SKL_MICS_RT_REGISTRY_H

#include <stdint.h>

#include <vector>

#include "utils/string_view.h"   // IWYU pragma: keep
#include "rt/type_info.h"   // IWYU pragma: keep

namespace mics::rt {

class Registry;
using RegistrationCallback = void (*)(Registry &);

class Registry {
public:
    static Registry &instance() noexcept {
        static Registry reg;
        return reg;
    }

    void register_type(const TypeInfo *info) noexcept {
        if (!info || info->type_id == INVALID_TYPE_ID) return;
        if (find_by_id(info->type_id)) return;
        _types.push_back(info);
    }

    void register_callback(RegistrationCallback cb) noexcept {
        if (!cb) return;
        _callbacks.push_back(cb);
        cb(*this);
    }

    void invoke_callbacks() noexcept {
        for (auto cb : _callbacks) {
            cb(*this);
        }
    }

    const TypeInfo *find_by_id(TypeId id) const noexcept {
        for (auto *t : _types) {
            if (t->type_id == id) return t;
        }
        return nullptr;
    }

    const TypeInfo *find_by_name(const char *name) const noexcept {
        if (!name) return nullptr;
        for (auto *t : _types) {
            if (t->name && mics::utils::string_view(t->name) == name) return t;
        }
        return nullptr;
    }

    size_t type_count() const noexcept { return _types.size(); }

    const TypeInfo *type_at(size_t index) const noexcept {
        if (index >= _types.size()) return nullptr;
        return _types[index];
    }

    template<typename F>
    void for_each(F &&fn) const {
        for (auto *t : _types)
            fn(t);
    }

    void clear() noexcept { _types.clear(); }

private:
    Registry() = default;
    std::vector<const TypeInfo *> _types;
    std::vector<RegistrationCallback> _callbacks;
};

}   // namespace mics::rt
#endif