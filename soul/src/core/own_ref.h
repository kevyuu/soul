#pragma once

#include "core/objops.h"
#include "core/type_traits.h"

namespace soul
{
  template <typename T>
  class OwnRef
  {
  public:
    OwnRef(T&& ref) : ref_(std::move(ref)) {} // NOLINT
    void store_at(T* location) { soul::construct_at(location, std::move(ref_)); }

  private:
    T&& ref_;
  };

  template <ts_copy T>
  class OwnRef<T>
  {
  public:
    OwnRef(const T& ref) : ref_(ref) {} // NOLINT
    void store_at(T* location) { soul::construct_at(location, ref_); }

  private:
    const T& ref_;
  };

} // namespace soul
