#pragma once

#include "core/builtins.h"

using namespace soul;

/// Vector string formatter.
template <typename T, u8 N>
struct std::formatter<soul::Vec<T, N>> // NOLINT
    : std::formatter<T>
{
  template <typename FormatContext>
  auto format(const soul::Vec<T, N>& v, FormatContext& ctx) const
  {
    auto out = ctx.out();
    for (int i = 0; i < N; ++i)
    {
      out = ::std::format_to(out, "{}", (i == 0) ? "{" : ", ");
      out = formatter<T>::format(v[i], ctx);
    }
    out = ::std::format_to(out, "}}");
    return out;
  }
};

template <typename T, u8 R, u8 C>
struct std::formatter<soul::Matrix<T, R, C>> // NOLINT
    : std::formatter<typename soul::Matrix<T, R, C>::RowType>
{
  using MatrixRowType = typename soul::Matrix<T, R, C>::RowType;

  template <typename FormatContext>
  auto format(const soul::Matrix<T, R, C>& matrix, FormatContext& ctx) const
  {
    auto out = ctx.out();
    for (int r = 0; r < R; ++r)
    {
      out = ::std::format_to(out, "{}", (r == 0) ? "{" : ", ");
      out = formatter<MatrixRowType>::format(matrix.row(r), ctx);
    }
    out = std::format_to(out, "}}");
    return out;
  }
};

;
