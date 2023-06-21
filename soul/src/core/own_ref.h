#pragma once

#include "core/objops.h"
#include "core/type_traits.h"

namespace soul
{
  template <typename T, bool swappable = false>
  class OwnRef
  {
  public:
    OwnRef(T&& ref) : ref_(std::move(ref)) {} // NOLINT
    void store_at(T* location) { soul::construct_at(location, std::move(ref_)); }
    void swap_at(T* location)
      requires swappable
    {
      T tmp = std::move(*location);
      soul::construct_at(location, std::move(ref_));
      ref_ = std::move(tmp);
    }
    auto const_ref() -> const T& { return ref_; }
    auto forward() -> OwnRef<T, swappable>&& { return std::move(*this); }

  private:
    T&& ref_;
  };

  template <ts_copy T, bool swappable>
  class OwnRef<T, swappable>
  {
  public:
    OwnRef(const T& ref) : ref_(ref) {} // NOLINT
    void store_at(T* location) { soul::construct_at(location, ref_); }
    void swap_at(T* location)
      requires swappable
    {
      T tmp = *location;
      *location = ref_;
      ref_ = tmp;
    }
    auto const_ref() -> const T& { return ref_; }
    auto forward() -> OwnRef<T, swappable> { return *this; }

  private:
    using ref_type = std::conditional_t<swappable, T, const T&>;
    ref_type ref_;
  };

} // namespace soul
