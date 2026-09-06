#ifndef SKL_UTILS_STRING_VIEW_H
#define SKL_UTILS_STRING_VIEW_H

#include <stddef.h>

namespace mics::utils {

class string_view {
public:
    constexpr string_view() noexcept
        : data_(nullptr)
        , size_(0) {}

    template<size_t N>
    constexpr string_view(const char (&str)[N]) noexcept
        : data_(str)
        , size_(N - 1) {}

    constexpr string_view(const char *data, size_t size) noexcept
        : data_(data)
        , size_(size) {}

    constexpr string_view(const char *str) noexcept
        : data_(str)
        , size_(_strlen(str)) {}

    constexpr const char *data() const noexcept { return data_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr bool operator==(const string_view &other) const noexcept {
        if (size_ != other.size_) return false;
        for (size_t i = 0; i < size_; ++i)
            if (data_[i] != other.data_[i]) return false;
        return true;
    }

    constexpr bool operator<(const string_view &other) const noexcept {
        size_t min = size_ < other.size_ ? size_ : other.size_;
        for (size_t i = 0; i < min; ++i) {
            if (data_[i] != other.data_[i]) return data_[i] < other.data_[i];
        }
        return size_ < other.size_;
    }

    constexpr const char *substr(size_t pos) const noexcept { return data_ + pos; }

    constexpr string_view substr(size_t pos, size_t count) const noexcept { return string_view(data_ + pos, count); }

private:
    const char *data_;
    size_t size_;

    static constexpr size_t _strlen(const char *s) {
        size_t len = 0;
        while (s && s[len])
            ++len;
        return len;
    }
};
}   // namespace mics::utils
#endif