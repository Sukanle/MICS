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
#ifndef SKL_UTILS_VECTOR_H
#define SKL_UTILS_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>
#include <iterator>

namespace Reflect::Utils {

template<typename T, typename... Args>
T *construct_at(T *p, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    ::new (static_cast<void *>(p)) T(std::forward<Args>(args)...);
    return std::launder(p);
}

template<class _Tp>
struct vector_builder;

struct vector_empty_t {
    explicit vector_empty_t() = default;
};
inline constexpr vector_empty_t vector_empty{};

template<class _Tp>
struct vector {
public:
    using value_type = _Tp;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = _Tp *;
    using const_pointer = const _Tp *;
    using reference = _Tp &;
    using const_reference = const _Tp &;
    using iterator = const _Tp *;
    using const_iterator = const _Tp *;
    using reverse_iterator = std::reverse_iterator<const _Tp *>;
    using const_reverse_iterator = std::reverse_iterator<const _Tp *>;

    vector() = delete;

    vector(const vector &) = delete;
    vector &operator=(const vector &) = delete;

    vector(vector &&__that) noexcept {
        _M_data = __that._M_data;
        _M_size = __that._M_size;
        _M_cap = __that._M_cap;
        __that._M_data = nullptr;
        __that._M_size = 0;
        __that._M_cap = 0;
    }

    vector &operator=(vector &&__that) noexcept {
        if (&__that == this) [[unlikely]]
            return *this;
        for (size_t __i = 0; __i != _M_size; __i++)
            std::destroy_at(&_M_data[__i]);
        if (_M_cap != 0) free(_M_data);
        _M_data = __that._M_data;
        _M_size = __that._M_size;
        _M_cap = __that._M_cap;
        __that._M_data = nullptr;
        __that._M_size = 0;
        __that._M_cap = 0;
        return *this;
    }

    explicit vector(vector_empty_t) noexcept {
        _M_data = nullptr;
        _M_size = 0;
        _M_cap = 0;
    }

    ~vector() noexcept {
        for (size_t __i = 0; __i != _M_size; __i++)
            std::destroy_at(&_M_data[__i]);
        if (_M_cap != 0) free(_M_data);
    }

    size_t size() const noexcept { return _M_size; }
    size_t capacity() const noexcept { return _M_cap; }
    bool empty() const noexcept { return _M_size == 0; }
    static constexpr size_t max_size() noexcept { return SIZE_MAX / sizeof(_Tp); }

    const _Tp &operator[](size_t __i) const noexcept { return _M_data[__i]; }

    const _Tp &at(size_t __i) const
#ifndef __cpp_exceptions
        noexcept
#endif
    {
#ifdef __cpp_exceptions
        if (__i >= _M_size) [[unlikely]]
            throw std::out_of_range("vector::at");
#else
        if (__i >= _M_size) [[unlikely]] {
            assert("out of range");
            std::terminate();
        }
#endif
        return _M_data[__i];
    }

    const _Tp &front() const noexcept { return *_M_data; }
    const _Tp &back() const noexcept { return _M_data[_M_size - 1]; }

    const _Tp *data() const noexcept { return _M_data; }
    const _Tp *cdata() const noexcept { return _M_data; }

    const _Tp *begin() const noexcept { return _M_data; }
    const _Tp *end() const noexcept { return _M_data + _M_size; }
    const _Tp *cbegin() const noexcept { return _M_data; }
    const _Tp *cend() const noexcept { return _M_data + _M_size; }

    std::reverse_iterator<const _Tp *> rbegin() const noexcept { return std::make_reverse_iterator(_M_data + _M_size); }
    std::reverse_iterator<const _Tp *> rend() const noexcept { return std::make_reverse_iterator(_M_data); }
    std::reverse_iterator<const _Tp *> crbegin() const noexcept {
        return std::make_reverse_iterator(_M_data + _M_size);
    }
    std::reverse_iterator<const _Tp *> crend() const noexcept { return std::make_reverse_iterator(_M_data); }

    std::vector<_Tp> to_std() const {
        std::vector<_Tp> __vec;
        __vec.reserve(_M_size);
        for (size_t __i = 0; __i != _M_size; __i++) {
            __vec.push_back(_M_data[__i]);
        }
        return __vec;
    }

    bool operator==(const vector &__that) const noexcept {
        return std::equal(this->begin(), this->end(), __that.begin(), __that.end());
    }
    bool operator!=(const vector &__that) const noexcept { return !(*this == __that); }
    bool operator<(const vector &__that) const noexcept {
        return std::lexicographical_compare(this->begin(), this->end(), __that.begin(), __that.end());
    }
    bool operator>(const vector &__that) const noexcept { return __that < *this; }
    bool operator<=(const vector &__that) const noexcept { return !(__that < *this); }
    bool operator>=(const vector &__that) const noexcept { return !(*this < __that); }

    struct Buffer {
        _Tp *data;
        size_t size;
    };

    Buffer release() noexcept {
        Buffer __buf{_M_data, _M_size};
        _M_data = nullptr;
        _M_size = 0;
        _M_cap = 0;
        return __buf;
    }

    void adopt(Buffer __buf) noexcept {
        for (size_t __i = 0; __i != _M_size; __i++) {
            std::destroy_at(&_M_data[__i]);
        }
        if (_M_cap != 0) {
            std::free(_M_data);
        }
        _M_data = __buf.data;
        _M_size = __buf.size;
        _M_cap = __buf.size;
    }

    void swap(vector &__that) noexcept {
        std::swap(_M_data, __that._M_data);
        std::swap(_M_size, __that._M_size);
        std::swap(_M_cap, __that._M_cap);
    }

    friend struct vector_builder<_Tp>;

private:
    _Tp *_M_data;
    size_t _M_size;
    size_t _M_cap;

    vector(_Tp *__data, size_t __size, size_t __cap) noexcept
        : _M_data(__data)
        , _M_size(__size)
        , _M_cap(__cap) {}

    static _Tp *_M_allocate(size_t __n)
#ifndef __cpp_exceptions
        noexcept
#endif

    {
        if (__n == 0) return nullptr;
        void *__p = malloc(__n * sizeof(_Tp));
#ifndef __cpp_exceptions
        if (!__p) throw std::bad_alloc();
#else
        if (!__p) {
            assert("malloc failed");
            std::terminate();
        }
#endif
        return static_cast<_Tp *>(__p);
    }

    static void _M_deallocate(_Tp *__p) noexcept { std::free(__p); }
};

template<class _Tp>
struct vector_builder {
public:
    using value_type = _Tp;
    using size_type = size_t;

    vector_builder() noexcept {
        _M_data = nullptr;
        _M_size = 0;
        _M_cap = 0;
    }

    ~vector_builder() noexcept {
        for (size_t __i = 0; __i != _M_size; __i++) {
            std::destroy_at(&_M_data[__i]);
        }
        if (_M_cap != 0) {
            std::free(_M_data);
        }
    }

    vector_builder(const vector_builder &) = delete;
    vector_builder &operator=(const vector_builder &) = delete;

    vector_builder(vector_builder &&__that) noexcept {
        _M_data = __that._M_data;
        _M_size = __that._M_size;
        _M_cap = __that._M_cap;
        __that._M_data = nullptr;
        __that._M_size = 0;
        __that._M_cap = 0;
    }

    vector_builder &operator=(vector_builder &&__that) noexcept {
        if (&__that == this) [[unlikely]]
            return *this;
        for (size_t __i = 0; __i != _M_size; __i++) {
            std::destroy_at(&_M_data[__i]);
        }
        if (_M_cap != 0) {
            std::free(_M_data);
        }
        _M_data = __that._M_data;
        _M_size = __that._M_size;
        _M_cap = __that._M_cap;
        __that._M_data = nullptr;
        __that._M_size = 0;
        __that._M_cap = 0;
        return *this;
    }

    bool reserve(size_t __n) noexcept {
        if (__n <= _M_cap) return true;
        __n = std::max(__n, _M_cap * 2);
        auto __old_data = _M_data;
        auto __old_cap = _M_cap;
        if (__n == 0) {
            _M_data = nullptr;
            _M_cap = 0;
        } else {
            _M_data = static_cast<_Tp *>(malloc(__n * sizeof(_Tp)));
            if (!_M_data) return false;
            _M_cap = __n;
        }
        if (__old_cap != 0) {
            for (size_t __i = 0; __i != _M_size; __i++)
                construct_at(&_M_data[__i], std::move_if_noexcept(__old_data[__i]));
            for (size_t __i = 0; __i != _M_size; __i++)
                std::destroy_at(&__old_data[__i]);
            free(__old_data);
        }
        return true;
    }

    bool push_back(const _Tp &__val) noexcept {
        if (_M_size + 1 >= _M_cap) [[unlikely]]
            if (!reserve(_M_size + 1)) return false;
        construct_at(&_M_data[_M_size], __val);
        _M_size = _M_size + 1;
        return true;
    }

    bool push_back(_Tp &&__val) noexcept {
        if (_M_size + 1 >= _M_cap) [[unlikely]]
            if (!reserve(_M_size + 1)) return false;
        construct_at(&_M_data[_M_size], std::move(__val));
        _M_size = _M_size + 1;
        return true;
    }

    template<class... Args>
    _Tp &emplace_back(Args &&...__args)
#ifndef __cpp_exceptions
        noexcept
#endif
    {
        if (_M_size + 1 >= _M_cap) [[unlikely]]
#ifdef __cpp_exceptions
            if (!reserve(_M_size + 1)) throw std::bad_alloc();
#else
            if (!reserve(_M_size + 1)) {
                assert("reserve failed");
                std::terminate();
            }
#endif
        _Tp *__p = &_M_data[_M_size];
        construct_at(__p, std::forward<Args>(__args)...);
        _M_size = _M_size + 1;
        return *__p;
    }

    void clear() noexcept {
        for (size_t __i = 0; __i != _M_size; __i++)
            std::destroy_at(&_M_data[__i]);
        _M_size = 0;
    }

    size_t size() const noexcept { return _M_size; }
    size_t capacity() const noexcept { return _M_cap; }
    bool empty() const noexcept { return _M_size == 0; }
    _Tp *data() noexcept { return _M_data; }
    const _Tp *data() const noexcept { return _M_data; }

    _Tp &operator[](size_t __i) noexcept { return _M_data[__i]; }
    const _Tp &operator[](size_t __i) const noexcept { return _M_data[__i]; }

    _Tp *begin() noexcept { return _M_data; }
    _Tp *end() noexcept { return _M_data + _M_size; }
    const _Tp *begin() const noexcept { return _M_data; }
    const _Tp *end() const noexcept { return _M_data + _M_size; }

    vector<_Tp> build() && noexcept {
        vector<_Tp> __result(_M_data, _M_size, _M_cap);
        _M_data = nullptr;
        _M_size = 0;
        _M_cap = 0;
        return __result;
    }

    void append_from_std(const std::vector<_Tp> &__other) noexcept {
        reserve(_M_size + __other.size());
        for (const auto &__val : __other)
            push_back(__val);
    }

private:
    _Tp *_M_data;
    size_t _M_size;
    size_t _M_cap;
};

template<class _Tp>
vector<_Tp> make_vector(const std::vector<_Tp> &__src) noexcept {
    vector_builder<_Tp> __b;
    __b.append_from_std(__src);
    return std::move(__b).build();
}

template<class _Tp>
vector<_Tp> make_vector(std::vector<_Tp> &&__src) noexcept {
    vector_builder<_Tp> __b;
    __b.reserve(__src.size());
    for (auto &__val : __src) {
        __b.push_back(std::move(__val));
    }
    return std::move(__b).build();
}

}   // namespace Reflect::Utils
#endif