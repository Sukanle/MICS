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
#ifndef SKL_MICS_CT_FP_H_
#define SKL_MICS_CT_FP_H_

#include "ct/base_fp.h"   // IWYU pragma: keep

namespace mics::ct::fp {

template<typename TypeList, template_constants N>
using nth = typename base_nth<TypeList, N>::type;
template<typename TypeList>
using head = typename base_head<TypeList>::type;
template<typename TypeList>
using tail = typename base_tail<TypeList>::type;
template<typename TypeList>
using other = typename base_other<TypeList>::type;

template<typename TypeList, typename T>
using push_front = base_push_front<T, TypeList>;

template<typename TypeList, typename T>
using push_back = base_push_back<T, TypeList>;
template<typename TypeList>
using pop_back = base_pop_back<TypeList>;
template<typename TypeList>
using pop_front = base_pop_front<TypeList>;

template<typename... Lists>
using concat = base_concat<Lists...>;


template<typename TypeList>
inline constexpr template_constants size = base_size<TypeList>::value;
template<typename TypeList, template<typename> class F>
inline constexpr template_constants count = base_count<TypeList, F, TypeList::count - 1>::value;

template<typename TypeList, template<typename> class F, typename T>
using map = typename base_map<TypeList, F, T>::type;
template<template<typename> class F, typename TypeList>
using transform = typename base_transform<F, TypeList>::type;
template<template<typename> class F, typename List>
using flat_map = typename base_flat_map<F, List>::type;
template<typename TypeList, template<typename...> class F>
using filter = typename base_filter<TypeList, F>::type;
template<typename TypeList, template<typename...> class F, typename Arg>
using filter_args = typename base_filter<TypeList, F, Arg>::type;

template<typename TypeList>
using unique = typename base_unique<TypeList>::type;
template<typename TypeList, typename Target, template_constants Index = 0>
inline constexpr std::make_signed_t<template_constants> find_index = base_find_index<TypeList, Target, Index>::value;
template<typename TypeList, typename Target>
using remove = typename base_remove<TypeList, Target>::type;

template<typename List, typename Init, template<typename, typename> class Func>
using fold = typename base_fold<List, Init, Func>::type;
}   // namespace mics::ct::fp
#endif