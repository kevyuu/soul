#pragma once

#include "core/type_traits.h"

namespace soul
{

  template <typename T, typename BorrowedT>
  struct BorrowTrait
  {
    static constexpr b8 available                       = false;
    static constexpr auto Borrow(const T&) -> BorrowedT = delete;
  };

  template <typename BorrowedT, typename T>
    requires(BorrowTrait<T, BorrowedT>::available)
  [[nodiscard]]
  inline constexpr auto borrow(const T& val) -> BorrowedT
  {
    return BorrowTrait<T, BorrowedT>::Borrow(val);
  }

} // namespace soul
