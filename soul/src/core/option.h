#pragma once

#include "core/objops.h"
#include "core/own_ref.h"
#include "core/panic.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/util.h"

#include <optional>

namespace soul
{

  struct option_construct {
    struct some_t {
    };
    struct init_generate_t {
    };
    static constexpr auto some = some_t{};
    static constexpr auto init_generate = init_generate_t{};
  };

  template <typename T, typename Arg>
  concept ts_option_and_then_fn = ts_invocable<T, Arg> && is_option_v<std::invoke_result_t<T, Arg>>;

  template <typeset T>
  class Option
  {

  private:
    struct Dummy_ {
      // NOLINTBEGIN(hicpp-use-equals-default, modernize-use-equals-default)
      constexpr Dummy_() noexcept {}
      // NOLINTEND(hicpp-use-equals-default, modernize-use-equals-default)
    };
    union {
      Dummy_ dummy_;
      std::remove_const_t<T> value_;
    };
    bool has_value_ = false;

    using val_ret_type = std::remove_cv_t<T>;

  public:
    using this_type = Option<T>;

    constexpr Option() noexcept : dummy_() {}
    constexpr Option(const this_type& other) noexcept
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr Option(this_type&& other) noexcept
      requires(can_trivial_move_v<T>)
    = default;
    constexpr Option(this_type&& other) noexcept
      requires(can_nontrivial_move_v<T>);

    constexpr auto operator=(const this_type& other) noexcept -> this_type&
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr auto operator=(this_type&& other) noexcept -> this_type&
      requires(can_trivial_move_v<T>)
    = default;
    constexpr auto operator=(this_type&& other) noexcept -> this_type&
      requires(can_nontrivial_move_v<T>);

    constexpr ~Option() noexcept
      requires(can_trivial_destruct_v<T>)
    = default;
    constexpr ~Option() noexcept
      requires(can_nontrivial_destruct_v<T>);

    [[nodiscard]]
    static constexpr auto some(OwnRef<T> val) noexcept -> this_type;

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto init_generate(Fn fn) noexcept -> this_type;

    constexpr void swap(this_type& other) noexcept
      requires(can_move_v<T> && can_swap_v<T>);

    [[nodiscard]]
    constexpr auto clone() const noexcept -> this_type
      requires(can_clone_v<T>);

    constexpr void clone_from(const Option& other) noexcept
      requires(can_clone_v<T>);

    [[nodiscard]]
    constexpr auto has_value() const -> bool;

    template <ts_fn<bool, const T&> Fn>
    [[nodiscard]]
    constexpr auto has_value_and(Fn fn) const -> bool;

    [[nodiscard]]
    constexpr auto
    operator->() & noexcept -> T*;

    [[nodiscard]]
    constexpr auto
    operator->() const& noexcept -> const T*;

    [[nodiscard]]
    constexpr auto
    operator->() && -> T* = delete;

    [[nodiscard]]
    constexpr auto
    operator->() const&& -> const T* = delete;

    [[nodiscard]]
    constexpr auto
    operator*() & noexcept -> T&;

    [[nodiscard]]
    constexpr auto
    operator*() const& noexcept -> const T&;

    [[nodiscard]]
    constexpr auto
    operator*() && = delete;

    [[nodiscard]]
    constexpr auto
    operator*() const&& = delete;

    [[nodiscard]]
    constexpr auto unwrap() && -> val_ret_type;

    template <ts_convertible_to<T> U>
    [[nodiscard]]
    constexpr auto unwrap_or(U&& default_val) && -> val_ret_type;

    template <ts_fn<std::remove_cv_t<T>> Fn>
    [[nodiscard]]
    constexpr auto unwrap_or_else(Fn fn) && -> val_ret_type;

    template <ts_option_and_then_fn<T&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, T&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) & -> FnReturnT;

    template <
      ts_option_and_then_fn<const T&> Fn,
      typeset FnReturnT = std::invoke_result_t<Fn, const T&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) const& -> FnReturnT;

    template <ts_option_and_then_fn<T&&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, T&&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) && -> FnReturnT;

    template <
      ts_option_and_then_fn<const T&&> Fn,
      typeset FnReturnT = std::invoke_result_t<Fn, const T&&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) const&& -> FnReturnT;

    template <ts_invocable<T&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, T&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) & -> Option<FnReturnT>;

    template <ts_invocable<const T&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, const T&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) const& -> Option<FnReturnT>;

    template <ts_invocable<T&&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, T&&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) && -> Option<FnReturnT>;

    template <ts_invocable<const T&&> Fn, typeset FnReturnT = std::invoke_result_t<Fn, const T&&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) const&& -> Option<FnReturnT>;

    constexpr void reset();

  private:
    constexpr Option(const Option& other)
      requires(can_clone_v<T> || can_nontrivial_copy_v<T>)
        : has_value_(other.has_value_)
    {
      if (has_value_) {
        duplicate_at(&value_, other.value_);
      }
    }

    constexpr auto operator=(const Option& other) -> Option&
      requires(can_clone_v<T> || can_nontrivial_copy_v<T>)
    {
      Option(other).swap(*this);
      return *this;
    }

    constexpr explicit Option(option_construct::some_t /* tag */, OwnRef<T> val) : has_value_(true)
    {
      val.store_at(&value_);
    }

    template <ts_generate_fn<T> Fn>
    constexpr explicit Option(option_construct::init_generate_t /* tag */, Fn fn) : has_value_(true)
    {
      generate_at(&value_, fn);
    }
  };

  template <typeset T>
  constexpr Option<T>::Option(this_type&& other) noexcept
    requires(can_nontrivial_move_v<T>)
      : has_value_(other.has_value_)
  {
    if (other.has_value_) {
      construct_at(&value_, std::move(other.value_));
      has_value_ = true;
    }
  }

  template <typeset T>
  constexpr auto Option<T>::operator=(this_type&& other) noexcept -> this_type&
    requires(can_nontrivial_move_v<T>)
  {
    if (other.has_value_) {
      if (has_value_) {
        value_ = std::move(other.value_);
      }
    } else {
      reset();
    }
    return *this;
  }

  template <typeset T>
  constexpr Option<T>::~Option() noexcept
    requires(can_nontrivial_destruct_v<T>)
  {
    if (has_value_) {
      destroy_at(&value_);
    }
  }

  template <typeset T>
  [[nodiscard]]
  constexpr auto Option<T>::some(OwnRef<T> val) noexcept -> this_type
  {
    return this_type(option_construct::some, val.forward());
  }

  template <typeset T>
  template <ts_generate_fn<T> Fn>
  [[nodiscard]]
  constexpr auto Option<T>::init_generate(Fn fn) noexcept -> this_type
  {
    return this_type(option_construct::init_generate, fn);
  }

  template <typeset T>
  constexpr void Option<T>::swap(this_type& other) noexcept
    requires(can_move_v<T> && can_swap_v<T>)
  {
    if (has_value_ == other.has_value_) {
      if (has_value_) {
        using std::swap;
        swap(value_, other.value_);
      }
    } else {
      auto& src = has_value_ ? *this : other;
      auto& dst = has_value_ ? other : *this;
      construct_at(&dst.value_, std::move(src.value_));
      dst.has_value_ = true;
      src.reset();
    }
  }

  template <typeset T>
  constexpr auto Option<T>::clone() const noexcept -> this_type
    requires(can_clone_v<T>)
  {
    return Option(*this);
  }

  template <typeset T>
  constexpr void Option<T>::clone_from(const Option& other) noexcept
    requires(can_clone_v<T>)
  {
    *this = other;
  }

  template <typeset T>
  constexpr auto Option<T>::has_value() const -> bool
  {
    return has_value_;
  }

  template <typeset T>
  template <ts_fn<bool, const T&> Fn>
  constexpr auto Option<T>::has_value_and(Fn fn) const -> bool
  {
    if (has_value_) {
      return true;
    }
    return std::forward<Fn>(fn)(value_);
  }

  template <typeset T>
  constexpr auto Option<T>::operator->() & noexcept -> T*
  {
    SOUL_ASSERT(0, has_value_);
    return &value_;
  }

  template <typeset T>
  constexpr auto Option<T>::operator->() const& noexcept -> const T*
  {
    SOUL_ASSERT(0, has_value_);
    return &value_;
  }

  template <typeset T>
  constexpr auto Option<T>::operator*() & noexcept -> T&
  {
    SOUL_ASSERT(0, has_value_);
    return value_;
  }

  template <typeset T>
  constexpr auto Option<T>::operator*() const& noexcept -> const T&
  {
    SOUL_ASSERT(0, has_value_);
    return value_;
  }

  template <typeset T>
  constexpr auto Option<T>::unwrap() && -> val_ret_type
  {
    return std::move(value_);
  }

  template <typeset T>
  template <ts_convertible_to<T> U>
  constexpr auto Option<T>::unwrap_or(U&& default_val) && -> val_ret_type
  {
    if (has_value_) {
      return std::move(value_);
    }
    return static_cast<val_ret_type>(std::forward<U>(default_val));
  }

  template <typeset T>
  template <ts_fn<std::remove_cv_t<T>> Fn>
  constexpr auto Option<T>::unwrap_or_else(Fn fn) && -> val_ret_type
  {
    return has_value_ ? std::move(value_) : std::invoke(std::forward<Fn>(fn));
  }

  template <typeset T>
  template <ts_option_and_then_fn<T&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::and_then(Fn fn) & -> FnReturnT
  {
    if (has_value_) {
      return std::invoke(std::forward<Fn>(fn), value_);
    }
    return FnReturnT{};
  }

  template <typeset T>
  template <ts_option_and_then_fn<const T&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::and_then(Fn fn) const& -> FnReturnT
  {
    if (has_value_) {
      return std::invoke(fn, value_);
    }
    return FnReturnT{};
  }

  template <typeset T>
  template <ts_option_and_then_fn<T&&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::and_then(Fn fn) && -> FnReturnT
  {
    if (has_value_) {
      return std::invoke(fn, std::move(value_));
    }
    return FnReturnT{};
  }

  template <typeset T>
  template <ts_option_and_then_fn<const T&&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::and_then(Fn fn) const&& -> FnReturnT
  {
    if (has_value_) {
      return std::invoke(fn, std::move(value_));
    }
    return FnReturnT{};
  }

  template <typeset T>
  template <ts_invocable<T&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::transform(Fn fn) & -> Option<FnReturnT>
  {
    if (has_value_) {
      return Option<FnReturnT>::init_generate([&, this] { return std::invoke(fn, value_); });
    }
    return Option<FnReturnT>();
  }

  template <typeset T>
  template <ts_invocable<const T&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::transform(Fn fn) const& -> Option<FnReturnT>
  {
    if (has_value_) {
      return Option<FnReturnT>::init_generate([&, this] { return std::invoke(fn, value_); });
    }
    return Option<FnReturnT>();
  }

  template <typeset T>
  template <ts_invocable<T&&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::transform(Fn fn) && -> Option<FnReturnT>
  {
    if (has_value_) {
      return Option<FnReturnT>::init_generate(
        [&, this] { return std::invoke(fn, std::move(value_)); });
    }
    return Option<FnReturnT>();
  }

  template <typeset T>
  template <ts_invocable<const T&&> Fn, typeset FnReturnT>
  constexpr auto Option<T>::transform(Fn fn) const&& -> Option<FnReturnT>
  {
    if (has_value_) {
      return Option<FnReturnT>::init_generate(
        [&, this] { return std::invoke(fn, std::move(value_)); });
    }
    return Option<FnReturnT>();
  }

  template <typeset T>
  constexpr void Option<T>::reset()
  {
    if (has_value_) {
      destroy_at(&value_);
      has_value_ = false;
    }
  }

  template <typeset T>
  constexpr void swap(Option<T>& left, Option<T>& right) noexcept
    requires(can_move_v<T> && can_swap_v<T>)
  {
    left.swap(right);
  }

  template <typeset T>
  constexpr auto operator==(const Option<T>& left, const Option<T>& right) noexcept -> bool
  {
    if (left.has_value() && right.has_value()) {
      return *left == *right;
    }
    return left.has_value() == right.has_value();
  }

} // namespace soul
