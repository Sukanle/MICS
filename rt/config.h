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
#ifndef SKL_MICS_RT_CONFIG_H
#define SKL_MICS_RT_CONFIG_H

#include <stdint.h>

namespace mics::rt {

// Type identity is a canonical two-word value. It must not be truncated to
// one word when crossing the runtime/ABIX metadata boundary.
struct TypeId {
    uint64_t lo = 0;
    uint64_t hi = 0;

    constexpr bool operator==(TypeId other) const noexcept {
        return lo == other.lo && hi == other.hi;
    }
    constexpr bool operator!=(TypeId other) const noexcept { return !(*this == other); }
};

using FieldId = uint32_t;
using MethodId = uint32_t;

constexpr TypeId INVALID_TYPE_ID{};

enum class Kind : uint8_t {
    Class,
    Struct,
    Enum,
    Primitive,
    Pointer,
    Reference,
};

enum class FieldKind : uint8_t {
    MemberVar,
    StaticVar,
    MemberFn,
    StaticFn,
};

enum class Visibility : uint8_t {
    Public,
    Protected,
    Private,
};

}   // namespace mics::rt
#endif
