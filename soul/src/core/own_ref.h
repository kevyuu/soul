#pragma once

#include "core/objops.h"
#include "core/type_traits.h"

namespace soul
{
  template <typename T>
  class OwnRef
  {
  public:
    constexpr OwnRef(T&& ref) : ref_(std::move(ref)) {} // NOLINT

    constexpr OwnRef(const OwnRef& other) = delete;

    constexpr OwnRef(OwnRef&& other) noexcept : ref_(std::move(other.ref_)) {}

    constexpr auto operator=(const OwnRef& other) = delete;

    constexpr auto operator=(OwnRef&& other) noexcept -> OwnRef&
    {
      swap(other);
      return *this;
    }

    constexpr ~OwnRef() = default;

    void swap(const OwnRef& other)
    {
      T tmp      = std::move(ref_);
      ref_       = std::move(other);
      other.ref_ = std::move(tmp);
    }

    friend void swap(const OwnRef& lhs, const OwnRef& rhs)
    {
      lhs.swap(rhs);
    }

    constexpr auto const_ref() -> const T&
    {
      return ref_;
    }

    constexpr operator T() &&
    {
      return std::move(ref_);
    } // NOLINT

  private:
    T&& ref_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  };

  template <ts_copy T>
  class OwnRef<T>
  {
  private:
    using ref_type = const T&;

  public:
    constexpr OwnRef(const T& ref) : ref_(ref) {} // NOLINT

    constexpr OwnRef(const OwnRef& other) = delete;

    constexpr OwnRef(OwnRef&& other) noexcept : ref_(other.ref_) {}

    constexpr auto operator=(const OwnRef& other) = delete;

    constexpr auto operator=(OwnRef&& other) noexcept -> OwnRef&
    {
      swap(other);
      return *this;
    }

    constexpr ~OwnRef() = default;

    void swap(const OwnRef& other)
    {
      swap(ref_, other.ref_);
    }

    friend void swap(const OwnRef& lhs, const OwnRef& rhs)
    {
      lhs.swap(rhs);
    }

    constexpr auto const_ref() -> const T&
    {
      return ref_;
    }

    constexpr operator T() &&
    {
      return ref_;
    } // NOLINT

  private:
    ref_type ref_;
  };

} // namespace soul
