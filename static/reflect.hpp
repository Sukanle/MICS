#pragma once
#include "base_reflect.hpp"
#include "fp.hpp"   // IWYU pragma: keep

namespace reflect::Static {

template<template<typename...> class F, typename... Args>
inline constexpr bool NOT = !F<Args...>::value;

template<class T,
         typename = std::enable_if_t<std::is_class_v<T> || std::is_enum_v<T>>>
struct TypeInfo;

template<typename T> struct is_type_list : std::false_type {};
template<typename... Ts>
struct is_type_list<type_list<Ts...>> : std::true_type {};

template<typename T>
inline constexpr bool is_type_list_v = is_type_list<T>::value;

template<typename T> struct cv_combinations {
    using type = type_list<T, std::add_const_t<T>, std::add_volatile_t<T>,
                           std::add_cv_t<T>>;
};
template<typename T> struct ref_combinations {
    using type = type_list<T, std::add_lvalue_reference_t<T>,
                           std::add_rvalue_reference_t<T>>;
};

template<typename T>
using cv_combinations_t = typename cv_combinations<T>::type;
template<typename T>
using ref_combinations_t = typename ref_combinations<T>::type;

// template<typename PtrT> struct compact_field_meta {
//     using pointer_type = PtrT;
//     pointer_type _ptr;
//     std::string_view _name;

//     [[nodiscard]] constexpr std::string_view getName() const noexcept {
//         return _name;
//     }
//     [[nodiscard]] constexpr std::string_view from_TempName() const noexcept {
//         return _name;
//     }
// };

// template<typename PtrT>
// [[nodiscard]] constexpr auto make_compact_field_meta(
//     PtrT ptr, std::string_view name) noexcept {
//     return compact_field_meta<PtrT>{ptr, name};
// }

#if !TEMPLATE_STRING_SUPPORTED
constexpr std::string_view strip_prefix(
    std::string_view name, std::string_view prefix = "&class_t::") noexcept {
    return name.substr(prefix.size());
}
#endif

template<typename T, typename Class = void, DEFAULT_TEMPLATE_STRING(Name, "")>
struct field_traits : __base_field_traits<T, is_Kind<T>(), Class, Name> {
    using traits = __base_field_traits<T, is_Kind<T>(), Class, Name>;
    explicit field_traits() = default;
    [[nodiscard]] std::string_view getName() const { return _name; }
    explicit field_traits(T& ptr, std::string_view name = "")
        : __base_field_traits<T, is_Kind<T>(), Class, Name>{ptr}
        , _name(name) {}
    explicit consteval field_traits(T&& ptr, std::string_view name = "")
        : __base_field_traits<T, is_Kind<T>(), Class, Name>{std::move(ptr)}
        , _name(name) {}

    [[nodiscard]] static consteval std::string_view from_TempName() {
#if !TEMPLATE_STRING_SUPPORTED
        static_assert(
            Name != nullptr,
            "This function [SRef::field_traits::from_TempName] must be "
            "an object constructed when passing in the second template "
            "argument to `SRef::field_traits` in order to be called; \n"
            "To fix this bug, pass in the second template variable, or "
            "call `SRef::field_trait obj.sgetName()`.");
#endif
        return NameAccessor<Name>();
    }

private:
    std::string_view _name;
};

template<typename T> consteval auto type_info() {
    return TypeInfo<T>{};
}
}   // namespace reflect::Static
