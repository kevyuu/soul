// clang-format off
[[nodiscard]] constexpr auto xx() const noexcept { return Vec<T, 2>(x, x); }
[[nodiscard]] constexpr auto xy() const noexcept { return Vec<T, 2>(x, y); }
[[nodiscard]] constexpr auto yx() const noexcept { return Vec<T, 2>(y, x); }
[[nodiscard]] constexpr auto yy() const noexcept { return Vec<T, 2>(y, y); }
// clang-format on
