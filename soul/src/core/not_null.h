#pragma once

#include "core/panic.h"
#include "core/type_traits.h"

namespace soul
{

  namespace impl
  {
    struct NotNullConstruct {
      struct NewUnchecked {
      };
      static constexpr auto new_unchecked = NewUnchecked{};
    };
  } // namespace impl

  template <ts_pointer T>
    requires(!is_const_v<T>)
  class NotNull;

  template <typename T, typename PtrT = match_any>
  inline constexpr b8 is_not_null_v = [] {
    if constexpr (!is_specialization_v<T, NotNull>) {
      return false;
    } else {
      return is_match_v<typename T::ptr_type, PtrT>;
    }
  }();

  template <typename T, typename PtrT = match_any>
  concept ts_not_null = is_not_null_v<T, PtrT>;

  template <ts_pointer T>
    requires(!is_const_v<T>)
  class NotNull
  {
  public:
    using ptr_type = T;
    constexpr NotNull(const NotNull& other) = default;
    constexpr auto operator=(const NotNull& other) -> NotNull& = default;
    constexpr NotNull(NotNull&& other) noexcept = default;
    constexpr auto operator=(NotNull&& other) noexcept -> NotNull& = default;
    constexpr ~NotNull() = default;

    constexpr NotNull(T ptr) : ptr_(ptr) { SOUL_ASSERT(0, ptr != nullptr); } // NOLINT

    constexpr void swap(NotNull& other)
    {
      const auto tmp_ptr = ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = tmp_ptr;
    }

    friend constexpr void swap(NotNull& lhs, NotNull& rhs) { lhs.swap(rhs); }

    constexpr static auto new_unchecked(T ptr) -> NotNull<T>
    {
      return NotNull(impl::NotNullConstruct::new_unchecked, ptr);
    }

    constexpr operator T() const { return ptr_; } // NOLINT
    constexpr auto operator*() const -> std::remove_pointer_t<T>& { return *ptr_; }
    constexpr auto operator->() const -> T { return ptr_; }

    // prevents compilation when someone attempts to assign a null pointer constant
    NotNull(std::nullptr_t) = delete;
    auto operator=(std::nullptr_t) -> NotNull& = delete;

    auto operator++() -> NotNull& = delete;
    auto operator--() -> NotNull& = delete;
    auto operator++(int) -> NotNull = delete;
    auto operator--(int) -> NotNull = delete;
    auto operator+=(std::ptrdiff_t) -> NotNull = delete;
    auto operator-=(std::ptrdiff_t) -> NotNull = delete;
    void operator[](std::ptrdiff_t) const = delete;

  private:
    constexpr explicit NotNull(impl::NotNullConstruct::NewUnchecked /* tag */, T val) : ptr_(val) {}
    T ptr_;
  };

  template <typename T>
  constexpr auto ptrof(T&& obj) -> NotNull<std::remove_reference_t<T>*>
  {
    return NotNull<std::remove_reference_t<T>*>::new_unchecked(std::addressof(obj));
  }

  // more unwanted operators
  template <ts_pointer T, ts_pointer U>
  auto operator-(const NotNull<T>&, const NotNull<U>&) -> std::ptrdiff_t = delete;
  template <ts_pointer T>
  auto operator-(const NotNull<T>&, std::ptrdiff_t) -> NotNull<T> = delete;
  template <ts_pointer T>
  auto operator+(const NotNull<T>&, std::ptrdiff_t) -> NotNull<T> = delete;
  template <ts_pointer T>
  auto operator+(std::ptrdiff_t, const NotNull<T>&) -> NotNull<T> = delete;

  template <typeset T>
  constexpr auto operator<=>(const NotNull<T>& lhs, const NotNull<T>& rhs) noexcept
  {
    return operator<=>(lhs.get(), rhs.get());
  }

} // namespace soul
