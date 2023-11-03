#pragma once

#include "core/objops.h"
#include "core/type_traits.h"

namespace soul
{
  template <typename T, b8 swappable = false>
  class OwnRef
  {
  public:
    constexpr OwnRef(T&& ref) : ref_(std::move(ref)) {} // NOLINT
    constexpr void store_at(T* location) { soul::construct_at(location, std::move(ref_)); }
    constexpr void swap_at(T* location)
      requires swappable
    {
      T tmp = std::move(*location);
      *location = std::move(ref_);
      ref_ = std::move(tmp);
    }
    constexpr auto const_ref() -> const T& { return ref_; }
    constexpr auto forward() -> OwnRef<T, swappable>&& { return std::move(*this); }
    constexpr auto forward_ref() -> T&& { return std::move(ref_); }

    constexpr operator T() { return std::move(ref_); } // NOLINT

  private:
    T&& ref_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  };

  template <ts_copy T, b8 swappable>
  class OwnRef<T, swappable>
  {
  private:
    using ref_type = std::conditional_t<swappable, T, const T&>;

  public:
    constexpr OwnRef(const T& ref) : ref_(ref) {} // NOLINT
    constexpr void store_at(T* location) { soul::construct_at(location, ref_); }
    constexpr void swap_at(T* location)
      requires swappable
    {
      T tmp = *location;
      *location = ref_;
      ref_ = tmp;
    }
    constexpr auto const_ref() -> const T& { return ref_; }
    constexpr auto forward() -> OwnRef<T, swappable> { return *this; }
    constexpr auto forward_ref() -> ref_type { return ref_; }
    constexpr operator T() { return ref_; } // NOLINT

  private:
    ref_type ref_;
  };

} // namespace soul
