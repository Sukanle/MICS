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
#ifndef SKL_MICS_RT_ANY_H
#define SKL_MICS_RT_ANY_H

#include <stdint.h>
#include <string.h>

#include <utility>
#include <type_traits>

#include "utils/type_hash.h"   // IWYU pragma: keep

#include "rt/config.h"   // IWYU pragma: keep

namespace mics::rt {

class Any {
    static constexpr size_t SBO_SIZE = 16;

    struct Handler {
        void *(*clone)(const void *src);
        void (*destroy)(void *ptr);
    };

public:
    Any() noexcept
        : _type_id(INVALID_TYPE_ID) {
        memset(_buf, 0, sizeof(_buf));
    }

    template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any>>>
    Any(T &&value) noexcept(std::is_nothrow_move_constructible_v<std::decay_t<T>>)
        : _type_id(INVALID_TYPE_ID) {
        construct(std::forward<T>(value));
    }

    Any(const Any &other) noexcept
        : _type_id(other._type_id) {
        if (other._handler.clone) {
            const void *src = other.ptr();
            void *dst = other._handler.clone(src);
            if (other.is_sbo()) memcpy(_buf, dst, sizeof(_buf));
            set_ptr(dst);
        } else
            memcpy(_buf, other._buf, sizeof(_buf));
    }

    Any(Any &&other) noexcept
        : _type_id(other._type_id) {
        memcpy(_buf, other._buf, sizeof(_buf));
        other._type_id = INVALID_TYPE_ID;
        other._handler = Handler{};
        memset(other._buf, 0, sizeof(other._buf));
    }

    Any &operator=(Any other) noexcept {
        swap(other);
        return *this;
    }

    ~Any() noexcept { destroy(); }

    void swap(Any &other) noexcept {
        std::swap(_type_id, other._type_id);
        std::swap(_handler, other._handler);
        char tmp[sizeof(_buf)];
        memcpy(tmp, _buf, sizeof(_buf));
        memcpy(_buf, other._buf, sizeof(_buf));
        memcpy(other._buf, tmp, sizeof(_buf));
    }

    friend void swap(Any &a, Any &b) noexcept { a.swap(b); }

    TypeId type_id() const noexcept { return _type_id; }
    bool has_value() const noexcept { return _type_id != INVALID_TYPE_ID; }
    explicit operator bool() const noexcept { return has_value(); }

    void *raw_ptr() noexcept { return is_sbo() ? static_cast<void *>(_buf) : _ptr; }
    const void *raw_ptr() const noexcept { return is_sbo() ? static_cast<const void *>(_buf) : _ptr_data; }

    template<typename T>
    T *try_cast() noexcept {
        if (_type_id != type_id_of<T>()) return nullptr;
        return static_cast<T *>(raw_ptr());
    }

    template<typename T>
    const T *try_cast() const noexcept {
        if (_type_id != type_id_of<T>()) return nullptr;
        return static_cast<const T *>(raw_ptr());
    }

    template<typename T>
    static TypeId type_id_of() noexcept {
        return mics::util::type_hash<T>();
    }

private:
    bool is_sbo() const noexcept { return _handler.clone == nullptr; }

    void *ptr() noexcept { return is_sbo() ? static_cast<void *>(_buf) : _ptr; }
    const void *ptr() const noexcept { return is_sbo() ? static_cast<const void *>(_buf) : _ptr_data; }

    void set_ptr(void *p) noexcept {
        _ptr = p;
        _ptr_data = static_cast<const void *>(p);
    }

    template<typename T>
    static void *clone_impl(const void *src) {
        auto *p = static_cast<const T *>(src);
        return new T(*p);
    }

    template<typename T>
    static void destroy_impl(void *ptr) {
        delete static_cast<T *>(ptr);
    }

    template<typename T>
    void construct(T &&value) {
        using DecayT = std::decay_t<T>;
        _type_id = type_id_of<DecayT>();
        if constexpr (sizeof(DecayT) <= SBO_SIZE && std::is_trivially_copyable_v<DecayT>) {
            _handler = Handler{};
            new (_buf) DecayT(std::forward<T>(value));
        } else {
            auto *p = new DecayT(std::forward<T>(value));
            _handler = Handler{&clone_impl<DecayT>, &destroy_impl<DecayT>};
            set_ptr(p);
        }
    }

    void destroy() noexcept {
        if (_type_id != INVALID_TYPE_ID && _handler.destroy) {
            _handler.destroy(ptr());
        }
        _type_id = INVALID_TYPE_ID;
        _handler = Handler{};
    }

    TypeId _type_id;
    Handler _handler;
    union {
        char _buf[SBO_SIZE];
        void *_ptr;
        const void *_ptr_data;
    };
};

}   // namespace mics::rt
#endif