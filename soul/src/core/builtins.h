#pragma once

#include <cstdint>

namespace soul
{

  using i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;

  using b8 = bool;

  using u8 = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;

  using f32 = float;
  static_assert(sizeof(f32) == 4);
  using f64 = double;
  static_assert(sizeof(f64) == 8);

  using iptr = std::intptr_t;
  using uptr = std::uintptr_t;

  using byte = u8;
  using usize = u64;

} // namespace soul
