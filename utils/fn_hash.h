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
#ifndef SKL_UTILS_FN_HASH_H
#define SKL_UTILS_FN_HASH_H

#include <stdint.h>

#include "utils/hash.h"        // IWYU pragma: keep
#include "utils/type_hash.h"   // IWYU pragma: keep

namespace Reflect::Utils {

template<hash64_t RetHash, hash64_t... ArgHashes>
constexpr hash64_t compute_fn_hash() noexcept {
    hash64_t h = cstr64("fn(");
    h = mix(h, RetHash);
    hash64_t acc = cstr64("(");
    ((acc = mix(acc, ArgHashes)), ...);
    if constexpr (sizeof...(ArgHashes) == 0) acc = cstr64("()");
    h = mix(h, acc);
    h = mix(h, cstr64(")"));
    return h;
}

}   // namespace Reflect::Utils

#endif   // SKL_UTILS_FN_HASH_H
