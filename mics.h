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
#ifndef SKL_MICS_H_
#define SKL_MICS_H_

#include "HLMD/HLMD.h"   // IWYU pragma: keep

#include "metadata.h"   // IWYU pragma: keep

#include "utils/hash.h"          // IWYU pragma: keep
#include "utils/type_hash.h"     // IWYU pragma: keep
#include "utils/string_view.h"   // IWYU pragma: keep

#include "ct/reflect.h"    // IWYU pragma: keep
#include "rt/reflect.h"   // IWYU pragma: keep

namespace SRefl = ::mics::ct;    // NOLINT
namespace DRefl = ::mics::rt;   // NOLINT
namespace URefl = ::mics::utils;     // NOLINT

#define SKL_RFS_MAKE_FIELD_TRAITS(type, VAR) SKL_RFS_MAKE_##type##_FIELD_TRAITS(VAR)
#define SKL_RFS_REGISTER_BEGIN(TYPE, ...)                             \
    template<>                                                        \
    struct SRefl::TypeInfo<TYPE, ##__VA_ARGS__> {                     \
        using class_t = TYPE;                                         \
        static constexpr URefl::string_view _name = #TYPE " [class]"; \
        struct Registry {
#define SKL_RFS_PROPERTY(MEM, ...)                                                                                    \
    static constexpr auto _##MEM = SRefl::field_traits<decltype(&class_t::MEM),                                       \
        class_t HLMD__VA_OPT__((, HLMD_GET_FIRST_ARGS(__VA_ARGS__)), (SKL_MAKE_TEMPLATE_STRING(#MEM)), __VA_ARGS__)>{ \
        &class_t::MEM, #MEM};
#define SKL_RFS_REGISTER_END() \
    }                          \
    ;                          \
    }                          \
    ;

#define SKL_RFS_ENUM_BEGIN(TYPE)                                                                                 \
    template<>                                                                                                   \
    struct SRefl::TypeInfo<TYPE> : SRefl::enum_traits<TYPE> {                                                    \
        using traits = enum_traits<TYPE>;                                                                        \
        using enum_t = traits::type;                                                                             \
        static constexpr URefl::string_view _name = traits::is_scoped ? #TYPE " [enum class]" : #TYPE " [enum]"; \
        static constexpr traits::Entry list[] = {

#define SKL_RFS_ENUM_VALUE(value, name) {value, name},
#define SKL_RFS_ENUM_END() \
    }                      \
    ;                      \
    }                      \
    ;
#define SKL_RFS_NOR_VAR(VAR) \
    SRefl::field_traits<decltype(VAR), SKL_MAKE_TEMPLATE_STRING(#VAR)> { VAR, #VAR }
#define SKL_RFS_OVR_FN(FN, FN_TYPE) \
    SRefl::field_traits<FN_TYPE, SKL_MAKE_TEMPLATE_STRING(#FN)> { FN, #FN }
#define SKL_RFS_NOR_FN(FN) SKL_RFS_OVR_FN(FN, decltype(FN))

#define SKL_RFD_REGISTER_BEGIN(ClassName)                                           \
    static void _dyn_refl_register_##ClassName();                                   \
    namespace {                                                                     \
    struct _dyn_refl_init_##ClassName {                                             \
        _dyn_refl_init_##ClassName() noexcept { _dyn_refl_register_##ClassName(); } \
    } _dyn_refl_init_inst_##ClassName;                                              \
    }                                                                               \
    static void _dyn_refl_register_##ClassName() {                                  \
        static bool _dyn_refl_registered_##ClassName = false;                       \
        if (_dyn_refl_registered_##ClassName) return;                               \
        _dyn_refl_registered_##ClassName = true;                                    \
        static DRefl::TypeInfo _dyn_refl_ti_##ClassName;                            \
        using Class_t = ClassName;                                                  \
        auto &_ti = _dyn_refl_ti_##ClassName;                                       \
        _ti.name = #ClassName;                                                      \
        _ti.type_id = DRefl::type_id_of<Class_t>();                                 \
        _ti.kind = DRefl::Kind::Class;                                              \
        _ti.size = sizeof(Class_t);                                                 \
        URefl::vector_builder<DRefl::BaseInfo> _dyn_refl_builder_bases;             \
        URefl::vector_builder<DRefl::FieldAccessor> _dyn_refl_builder_fields;       \
        URefl::vector_builder<DRefl::FnInfo> _dyn_refl_builder_methods;

#define SKL_RFD_FIELD(member)                                                                                          \
    {                                                                                                                  \
        using _dyn_refl_cls_ = Class_t;                                                                                \
        constexpr auto _mp = &_dyn_refl_cls_::member;                                                                  \
        DRefl::FieldAccessor _acc;                                                                                     \
        _acc.info = DRefl::FieldInfo(#member, DRefl::type_id_of<_dyn_refl_cls_>(),                                     \
            static_cast<uint32_t>(offsetof(Class_t, member)), DRefl::FieldKind::MemberVar, DRefl::Visibility::Public); \
        _acc.getter = DRefl::detail::make_field_getter<_mp>();                                                         \
        _acc.setter = DRefl::detail::make_field_setter<_mp>();                                                         \
        _dyn_refl_builder_fields.push_back(_acc);                                                                      \
    }

#define SKL_RFD_METHOD(method)                                               \
    {                                                                        \
        using _dyn_refl_cls_ = Class_t;                                      \
        auto _mfp = &_dyn_refl_cls_::method;                                 \
        DRefl::FnInfo _fn;                                                   \
        _fn.name = #method;                                                  \
        _fn.is_static = false;                                               \
        _fn.visibility = DRefl::Visibility::Public;                          \
        _fn.invoker = [](void *obj, void ** /*args*/, void * /*result*/) {}; \
        _dyn_refl_builder_methods.push_back(_fn);                            \
    }

#define SKL_RFD_REGISTER_END()                                  \
    _ti.bases = std::move(_dyn_refl_builder_bases).build();     \
    _ti.fields = std::move(_dyn_refl_builder_fields).build();   \
    _ti.methods = std::move(_dyn_refl_builder_methods).build(); \
    DRefl::Registry::instance().register_type(&_ti);            \
    }

#define SKL_RFD_ENUM_BEGIN(EnumName)                                                            \
    static void _dyn_refl_register_enum_##EnumName();                                           \
    namespace {                                                                                 \
    struct _dyn_refl_init_enum_##EnumName {                                                     \
        _dyn_refl_init_enum_##EnumName() noexcept { _dyn_refl_register_enum_##EnumName(); }     \
    } _dyn_refl_init_enum_inst_##EnumName;                                                      \
    }                                                                                           \
    static void _dyn_refl_register_enum_##EnumName() {                                          \
        static bool _dyn_refl_enum_registered_##EnumName = false;                               \
        if (_dyn_refl_enum_registered_##EnumName) return;                                       \
        _dyn_refl_enum_registered_##EnumName = true;                                            \
        static DRefl::EnumInfo _dyn_refl_ei_##EnumName;                                         \
        using EnumName_t = EnumName;                                                            \
        auto &_ei = _dyn_refl_ei_##EnumName;                                                    \
        _ei.name = #EnumName;                                                                   \
        _ei.type_id = DRefl::type_id_of<EnumName_t>();                                          \
        _ei.underlying_type_id = DRefl::type_id_of<std::underlying_type_t<EnumName_t>>();       \
        _ei.is_scoped = !std::is_convertible_v<EnumName_t, std::underlying_type_t<EnumName_t>>; \
        static DRefl::TypeInfo _dyn_refl_eti_##EnumName;                                        \
        auto &_eti = _dyn_refl_eti_##EnumName;                                                  \
        _eti.name = #EnumName;                                                                  \
        _eti.type_id = _ei.type_id;                                                             \
        _eti.kind = DRefl::Kind::Enum;                                                          \
        _eti.size = sizeof(EnumName_t);                                                         \
        _eti.enum_info = &_ei;                                                                  \
        URefl::vector_builder<DRefl::EnumEntry> _dyn_refl_builder_entries;

#define SKL_RFD_ENUM_VALUE(value, name) _dyn_refl_builder_entries.push_back({static_cast<int64_t>(value), name});

#define SKL_RFD_ENUM_END()                                      \
    _ei.entries = std::move(_dyn_refl_builder_entries).build(); \
    DRefl::Registry::instance().register_type(&_eti);           \
    }

// =============================================================================
// Simple Registration Macro
// =============================================================================
#define SKL_RFS_ENUM(...) \
    HLMD__VA_OPT__(HLMD_DEFER(SKL_RFS_ENUM_BEGIN)(HLMD_GET_FIRST_ARGS(__VA_ARGS__)), SKL_RFS_ENUM_END(), __VA_ARGS__)
#define SKL_RFS_CLASS(...) \
    HLMD__VA_OPT__(        \
        HLMD_DEFER(SKL_RFS_REGISTER_BEGIN)(HLMD_GET_FIRST_ARGS(__VA_ARGS__)), SKL_RFS_REGISTER_END(), __VA_ARGS__)

#define SKL_RFD_ENUM(...) \
    HLMD__VA_OPT__(HLMD_DEFER(SKL_RFD_ENUM_BEGIN)(HLMD_GET_FIRST_ARGS(__VA_ARGS__)), SKL_RFD_ENUM_END(), __VA_ARGS__)
#define SKL_RFD_CLASS(...) \
    HLMD__VA_OPT__(        \
        HLMD_DEFER(SKL_RFD_REGISTER_BEGIN)(HLMD_GET_FIRST_ARGS(__VA_ARGS__)), SKL_RFD_REGISTER_END(), __VA_ARGS__)

#define SKL_RFD_PROPERTY(member, ...) SKL_RFD_FIELD(member) HLMD_EMPTY_ARG(__VA_ARGS__)
#undef SKL_RFD_METHOD
#define SKL_RFD_METHOD(method, ...)                                               \
    {                                                                             \
        DRefl::FnInfo _fn;                                                        \
        _fn.name = #method;                                                       \
        _fn.is_static = false;                                                    \
        _fn.visibility = DRefl::Visibility::Public;                               \
        _fn.invoker = [](void * /*obj*/, void ** /*args*/, void * /*result*/) {}; \
        _dyn_refl_builder_methods.push_back(_fn);                                 \
    }                                                                             \
    HLMD_EMPTY_ARG(__VA_ARGS__)

#endif