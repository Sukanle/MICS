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
#ifndef SKL_REFLECT_DYNAMIC_FN_INFO_H
#define SKL_REFLECT_DYNAMIC_FN_INFO_H

#include <stdint.h>

#include "utils/vector.h"     // IWYU pragma: keep
#include "dynamic/config.h"   // IWYU pragma: keep

namespace Reflect::Dynamic {

using MethodInvoker = void (*)(void *obj, void **args, void *result);

struct ParamInfo {
    const char *name;
    TypeId type_id;
};

struct FnInfo {
    const char *name;
    TypeId return_type_id;
    Utils::vector<ParamInfo> params;
    MethodInvoker invoker;
    Visibility visibility;
    bool is_const;
    bool is_static;

    FnInfo() noexcept
        : name(nullptr)
        , return_type_id(INVALID_TYPE_ID)
        , params(Utils::vector_empty)
        , invoker(nullptr)
        , visibility(Visibility::Public)
        , is_const(false)
        , is_static(false) {}

    FnInfo(const char *n, TypeId ret_tid, Utils::vector<ParamInfo> p, MethodInvoker inv,
        Visibility vis = Visibility::Public, bool cnst = false, bool st = false) noexcept
        : name(n)
        , return_type_id(ret_tid)
        , params(std::move(p))
        , invoker(inv)
        , visibility(vis)
        , is_const(cnst)
        , is_static(st) {}
};

}   // namespace Reflect::Dynamic
#endif