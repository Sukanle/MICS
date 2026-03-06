#pragma once
#include "HLMD/HLMD.h"      // IWYU pragma: keep
#include "static/reflect.hpp"   // IWYU pragma: keep

namespace SRefl = reflect::Static;
// namespace DRefl = reflect::Dynamic;

// clang-format off
#define MAKE_OBJ_FIELD_TRAITS(VAR) &class_t::VAR, #VAR
#define MAKE_NOR_FIELD_TRAITS(VAR) VAR, #VAR
#define MAKE_FIELD_TRAITS(type, VAR) MAKE_##type##_FIELD_TRAITS(VAR)
#define RFS_OBJ_BEGIN(TYPE,...)                         \
    template<> struct SRefl::TypeInfo<TYPE, ##__VA_ARGS__> {           \
        using class_t= TYPE;                            \
        static constexpr std::string_view _name = #TYPE \
        " [class]";                                     \
        struct Registry {
#define RFS_OBJ_BASE(BASE, IS_VIRTUAL) \
    static constexpr SRefl:
#define RFS_OBJ_MEM(MEM)                              \
    static constexpr auto _##MEM=                     \
        SRefl::field_traits<decltype(&class_t::MEM),  \
        class_t MAKE_TEMPLATE_STRING(#MEM)>{MAKE_OBJ_FIELD_TRAITS(MEM)};
#define RFS_OBJ_MEM_TEM(MEM, NAME) \
    static constexpr auto _##MEM=  \
        SRefl::field_traits<decltype(&class_t::MEM), class_t, NAME>{MAKE_OBJ_FIELD_TRAITS(MEM)};
#define RFS_OBJ_END() \
        };            \
    };

// #define MAKE_OBJ_FIELD_TRAITS(VAR) &class_t::VAR, #VAR
// #define MAKE_NOR_FIELD_TRAITS(VAR) VAR, #VAR
// #define MAKE_FIELD_TRAITS(type, VAR) MAKE_##type##_FIELD_TRAITS(VAR)
// #if defined(ENABLE_REFLECT_SKIP)
//     #if defined(RELECT_DEFAULT_REGISTER) || defined(REFLECT_DEFAULT_REGISTER)
//         #define RFS_AUTO_REGISTER_DEFAULT 1
//     #else
//         #define RFS_AUTO_REGISTER_DEFAULT 0
//     #endif
// #else
//     #define RFS_AUTO_REGISTER_DEFAULT 1
// #endif

// #define RFS_OBJ_BEGIN(TYPE,...)                         \
//     template<> struct SRefl::TypeInfo<TYPE, ##__VA_ARGS__> {           \
//         using class_t= TYPE;                            \
//         static constexpr std::string_view _name = #TYPE \
//         " [class]";                                     \
//         struct Registry {
// #define RFS_OBJ_BASE(BASE, IS_VIRTUAL) \
//     static constexpr SRefl:
// #define RFS_OBJ_REG(MEM)                              \
//     static constexpr auto _##MEM=                     \
//         SRefl::make_compact_field_meta(MAKE_OBJ_FIELD_TRAITS(MEM));
// #define RFS_OBJ_REG_TEM(MEM, NAME) \
//     static constexpr auto _##MEM=  \
//         SRefl::make_compact_field_meta(&class_t::MEM, SRefl::NameAccessor<NAME>());
// #if RFS_AUTO_REGISTER_DEFAULT
//     #define RFS_OBJ_MEM(MEM) RFS_OBJ_REG(MEM)
//     #define RFS_OBJ_MEM_TEM(MEM, NAME) RFS_OBJ_REG_TEM(MEM, NAME)
// #else
//     #define RFS_OBJ_MEM(MEM)
//     #define RFS_OBJ_MEM_TEM(MEM, NAME)
// #endif
// #define RFS_OBJ_END() \
//         };            \
//     };


#define RFS_ENUM_BEGIN(TYPE)                                            \
    template<> struct SRefl::TypeInfo<TYPE> : SRefl::enum_traits<TYPE>{ \
        using traits = enum_traits<TYPE>;                               \
        using enum_t = traits::type;                                    \
        static constexpr std::string_view _name = traits::is_scoped     \
        ? #TYPE " [enum class]" : #TYPE " [enum]";                      \
    };                                                                  \
    template<>                                                          \
    std::map<TYPE, std::string_view> SRefl::TypeInfo<TYPE>::traits::list{

#define RFS_ENUM_LIST(...) \
        __VA_ARGS__
#define RFS_ENUM_END() \
    };
#define RFS_NOR_VAR(VAR)                                           \
    SRefl::field_traits<decltype(VAR), MAKE_TEMPLATE_STRING(#VAR)> \
        {MAKE_NOR_FIELD_TRAITS(VAR)}
#define RFS_OVR_FN(FN, FN_TYPE)                             \
    SRefl::field_traits<FN_TYPE, MAKE_TEMPLATE_STRING(#FN)> \
        {MAKE_NOR_FIELD_TRAITS(FN)}
#define RFS_NOR_FN(FN) RFS_OVR_FN(FN, decltype(FN))
// clang-format on
