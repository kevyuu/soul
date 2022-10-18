/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/

#ifndef MI_BASE_ENUM_UTIL_H
#define MI_BASE_ENUM_UTIL_H

#if (__cplusplus >= 201103L)
#include "config.h"

#include <type_traits>

// internal utility for MI_MAKE_ENUM_BITOPS
#define MI_MAKE_ENUM_BITOPS_PAIR(Enum,OP) \
MI_HOST_DEVICE_INLINE constexpr Enum operator OP(const Enum l, const Enum r) { \
    using Basic = typename std::underlying_type<Enum>::type; \
    return static_cast<Enum>(static_cast<Basic>(l) OP static_cast<Basic>(r)); } \
MI_HOST_DEVICE_INLINE Enum& operator OP##=(Enum& l, const Enum r) { \
    using Basic = typename std::underlying_type<Enum>::type; \
    return reinterpret_cast<Enum&>(reinterpret_cast<Basic&>(l) OP##= static_cast<Basic>(r)); }


/// Utility to define binary operations on enum types.
///
/// Note that the resulting values may not have names in the given enum type.
#define MI_MAKE_ENUM_BITOPS(Enum) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,|) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,&) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,^) \
MI_HOST_DEVICE_INLINE constexpr Enum operator ~(const Enum e) { \
    return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(e)); }

#else
#define MI_MAKE_ENUM_BITOPS(Enum)
#endif

#endif //MI_BASE_ENUM_UTIL_H
